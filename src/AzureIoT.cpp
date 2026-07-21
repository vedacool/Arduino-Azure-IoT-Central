#include "AzureIoT.h"
#include "platform.h"
#include <PubSubClient.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "sas_token.h"
#include "dps_client.h"
#include "reset.h"
#include "b64url.h"

static SecureWiFiClient s_wifiClient;
static PubSubClient s_mqttClient(s_wifiClient);

// ---------------------------------------------------------------------
// Credentials, copied in by begin() (see AzureIoT.h for why this library
// takes them as arguments instead of reading a sketch-local config.h
// itself). Sized generously: SSID (802.11 max 32 bytes), WPA2 passphrase
// (max 63 bytes), DPS ID scope (typically ~11 chars, generous headroom),
// device ID, and device key (base64 -- sized to match what
// build_sas_token()/base64_decode() can actually handle without silently
// truncating a legitimately long key).
// ---------------------------------------------------------------------
static char s_wifiSsid[33] = {0};
static char s_wifiPassword[64] = {0};
static char s_idScope[32] = {0};
static char s_deviceIdInput[64] = {0}; // the ID used to CALL dpsProvision()
static char s_deviceKey[200] = {0};
static char s_modelId[128] = {0}; // optional -- see begin()'s modelId parameter

// ---- Tuning, overridable via the setters in AzureIoT.h before begin() ----
static char s_dpsGlobalHost[64] = "global.azure-devices-provisioning.net";
static unsigned long s_sasTokenLifetimeSecs = 3600UL;
static unsigned long s_sendIntervalMs = 5000UL;
static unsigned long s_wifiConnectTimeoutMs = 20000UL;
static unsigned long s_mqttReconnectCooldownMs = 5000UL;
static unsigned long s_wifiHardReinitAfterMs = 60000UL;
static unsigned long s_wifiForceResetAfterMs = 300000UL;
static unsigned long s_provisionRetryInitialMs = 60000UL;
static unsigned long s_provisionRetryMaxMs = 900000UL;

// ---- Runtime state ----
static char s_iotHubHost[128] = {0};
static char s_deviceId[64] = {0}; // the ID DPS actually assigned us (see provisionDevice())
static char s_sasToken[300] = {0};
static unsigned long s_sasTokenExpiry = 0;

static unsigned long s_lastFlushMillis = 0;
static unsigned long s_lastMqttAttemptMillis = 0;

// Staged telemetry -- publish() writes here, flush() reads it and clears it.
struct StagedValue { const char *key; float value; bool set; };
static StagedValue s_staged[16];
static size_t s_stagedCount = 0;

// Writable boolean properties -- onBoolProperty() registers here; the MQTT
// message callback (see mqttMessageCallback() below) matches incoming twin
// data against this table by name and invokes the matching callback.
struct BoolPropertyReg { const char *name; AzureIoTClass::BoolPropertyCallback callback; };
static BoolPropertyReg s_boolProps[16];
static size_t s_boolPropCount = 0;
static unsigned long s_twinRequestId = 0;
static bool s_watchdogEnabled = false; // set by enableWatchdog() -- gates every platformWatchdogPet() call

// reportBoolProperty()'s retry buffer -- if MQTT isn't connected (or the
// send otherwise fails) when reportBoolProperty() is called, the change is
// staged here instead of being lost outright, and retried from loop() once
// MQTT reconnects. Same overwrite semantics as s_staged[] above: if the
// same property changes again before the retry goes out, only the latest
// value matters, so this is one slot per distinct property name, not a
// growing queue of every historical change -- fixed size, no heap, same
// reasoning as every other table in this file. Independent from
// s_boolProps[] above (which is onBoolProperty()'s CLOUD -> device
// registration table) since reportBoolProperty() doesn't require a prior
// onBoolProperty() registration for the same name -- decoupled on purpose,
// matching how publish()'s keys are independent of anything else.
struct PendingReport { const char *name; bool value; bool pending; };
static PendingReport s_pendingReports[16];
static size_t s_pendingReportCount = 0;

// Forward declarations -- defined further down (near the other JSON
// helpers), but referenced from ensureMqttConnected() above that point.
static void requestFullTwin();
static void mqttMessageCallback(char *topic, uint8_t *payload, unsigned int length);
static void retryPendingReports();

static void connectWiFi() {
    // Tracks cumulative time since Wi-Fi was last known good, across
    // however many WiFi.begin() attempts it takes -- not reset per-attempt.
    // Two escalating remedies, both free of any Azure/DPS cost since
    // neither one gets anywhere near the network until Wi-Fi is actually
    // up:
    //   ~1 min:  reinitialize the Wi-Fi radio + fresh WiFi.begin() -- clears
    //            a wedged module/driver state that a plain retry can't.
    //   ~5 min:  full device reset -- last resort if even the reinit didn't
    //            help. Uses RSTCTRL.SWRR on megaAVR boards, ESP.restart()
    //            on ESP32 -- see reset.cpp.
    unsigned long downSince = millis();
    bool didHardReinit = false;

    while (true) {
        Serial.print("Connecting to WiFi: ");
        Serial.println(s_wifiSsid);
        WiFi.begin(s_wifiSsid, s_wifiPassword);

        unsigned long start = millis();
        bool restartAttempt = false;
        while (WiFi.status() != WL_CONNECTED && millis() - start < s_wifiConnectTimeoutMs) {
            delay(500);
            if (s_watchdogEnabled) platformWatchdogPet(); // this loop can run for minutes (see the reinit/reset tiers below) -- must not starve the watchdog
            Serial.print(".");

            if (millis() - downSince > s_wifiForceResetAfterMs) {
                Serial.println();
                Serial.println("WiFi still down after the reset threshold -- forcing a full device reset.");
                forceReset();
            }
            if (!didHardReinit && millis() - downSince > s_wifiHardReinitAfterMs) {
                Serial.println();
                Serial.println("WiFi still down after the reinit threshold -- reinitializing the WiFi module...");
                platformWifiHardReinit();
                didHardReinit = true;
                restartAttempt = true;
                break;
            }
        }
        if (WiFi.status() == WL_CONNECTED) break;
        if (restartAttempt) continue; // go straight back to a fresh WiFi.begin()

        Serial.println();
        Serial.println("WiFi connect timed out -- check the SSID/password passed to AzureIoT.begin(). Retrying...");
    }
    Serial.println();
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());

    // Wait for a real clock -- SAS tokens are time-based and Azure rejects
    // tokens whose expiry looks wrong, so we must not sign anything before
    // this. WiFiNINA modules sync automatically once connected; ESP32 needs
    // an explicit NTP kick (platformBeginTimeSync() below) first.
    platformBeginTimeSync();
    Serial.print("Waiting for network time");
    // Deliberately NOT petting the watchdog in this loop, unlike the
    // Wi-Fi-connect wait above: that loop has its own escalation tiers
    // (reinit, then reset) if Wi-Fi never comes up, but THIS loop has no
    // timeout or fallback of its own -- if NTP is genuinely unreachable
    // (Wi-Fi connected, but no route to the internet), it would otherwise
    // hang forever with zero protection. Leaving it un-petted means the
    // watchdog (if enabled) is the one thing that can still catch and
    // recover from that specific case, rather than being fed through it.
    while (platformGetUnixTime() < 1700000000UL) { // sanity floor: after Nov 2023
        Serial.print(".");
        delay(500);
    }
    Serial.println();
}

static bool provisionDevice() {
    Serial.println("Provisioning via DPS...");
    DpsResult result;
    if (!dpsProvision(s_idScope, s_deviceIdInput, s_deviceKey, s_dpsGlobalHost, s_modelId, result)) {
        Serial.println("DPS provisioning FAILED.");
        return false;
    }
    strncpy(s_iotHubHost, result.assignedHub, sizeof(s_iotHubHost) - 1);
    strncpy(s_deviceId, result.deviceId, sizeof(s_deviceId) - 1);
    Serial.print("Assigned hub: ");
    Serial.println(s_iotHubHost);
    return true;
}

static bool refreshSasTokenIfNeeded() {
    unsigned long now = platformGetUnixTime();
    if (s_sasToken[0] != '\0' && now < s_sasTokenExpiry - 60) {
        return true; // still valid for at least another minute
    }
    // 220, not 192: s_iotHubHost (127 chars max) + "/devices/" (9) +
    // s_deviceId (63 chars max) + NUL = 200 -- 192 left only 8 bytes of
    // margin, and GCC's own -Wformat-truncation flagged this as a real
    // truncation risk (not hypothetical) if both fields were ever near
    // their actual maximums simultaneously. Not a memory-safety bug
    // (snprintf bounds itself), but a truncated resourceUri here would
    // sign a SAS token against the wrong resource string, which Azure
    // would then reject -- a real, if low-probability, functional bug.
    char resourceUri[220];
    int n = snprintf(resourceUri, sizeof(resourceUri), "%s/devices/%s", s_iotHubHost, s_deviceId);
    if (n <= 0 || (size_t)n >= sizeof(resourceUri)) {
        Serial.println("AzureIoT: internal error building resource URI for SAS token -- aborting refresh.");
        return false;
    }
    unsigned long expiry = now + s_sasTokenLifetimeSecs;
    if (build_sas_token(s_deviceKey, resourceUri, expiry, s_sasToken, sizeof(s_sasToken)) == 0) {
        Serial.println("Failed to build SAS token.");
        return false;
    }
    s_sasTokenExpiry = expiry;
    return true;
}

bool AzureIoTClass::ensureMqttConnected() {
    if (s_mqttClient.connected()) return true;
    if (millis() - s_lastMqttAttemptMillis < s_mqttReconnectCooldownMs) return false;
    s_lastMqttAttemptMillis = millis();

    if (!refreshSasTokenIfNeeded()) return false;

    s_mqttClient.setServer(s_iotHubHost, 8883);
    // NOTE: setBufferSize() is called ONCE in begin(), not here. PubSubClient's
    // setBufferSize() does a heap malloc()/realloc() -- calling it on every
    // reconnect (which will happen periodically over weeks of normal Wi-Fi
    // blips) would mean needless repeated heap churn on a 6KB-RAM board for
    // no benefit, since the size never changes.

    // 700, not 220: with a model-id appended, worst case is
    // s_iotHubHost (127) + s_deviceId (63) + the fixed "&model-id=" text +
    // a fully percent-encoded 128-byte model ID (every character encoded,
    // 3x expansion -- DTMI strings only ever need ':'/';' escaped in
    // practice, but sized for the true worst case rather than the typical
    // one, same reasoning as every other buffer fix this session).
    char username[700];
    int usernameLen;
    if (s_modelId[0] != '\0') {
        char modelIdEnc[400]; // 128 * 3 + margin
        azureiot_url_encode(s_modelId, modelIdEnc, sizeof(modelIdEnc));
        usernameLen = snprintf(username, sizeof(username), "%s/%s/?api-version=2021-04-12&model-id=%s",
                                s_iotHubHost, s_deviceId, modelIdEnc);
    } else {
        usernameLen = snprintf(username, sizeof(username), "%s/%s/?api-version=2021-04-12",
                                s_iotHubHost, s_deviceId);
    }
    if (usernameLen <= 0 || (size_t)usernameLen >= sizeof(username)) {
        Serial.println("AzureIoT: internal error building MQTT username -- aborting connect.");
        return false;
    }

    Serial.print("Connecting to MQTT as ");
    Serial.println(s_deviceId);
    if (s_mqttClient.connect(s_deviceId, username, s_sasToken)) {
        Serial.println("MQTT connected.");
        if (s_boolPropCount > 0) {
            // Only pay for the subscribe + twin GET round-trip if the sketch
            // actually registered a property -- sketches that only publish()
            // telemetry never touch this at all.
            s_mqttClient.subscribe("$iothub/twin/PATCH/properties/desired/#");
            s_mqttClient.subscribe("$iothub/twin/res/#");
            requestFullTwin();
        }
        return true;
    }
    Serial.print("MQTT connect failed, rc=");
    Serial.println(s_mqttClient.state());
    return false;
}

// Formats `value` to 2 decimal places into buf, using ONLY integer
// formatting (%ld, never %f). This isn't a style choice -- AVR's default
// C library does NOT support %f in snprintf() at all: avr-libc's own
// manual documents that the default (and minimized) vfprintf()
// implementations skip floating-point conversions entirely unless the
// sketch is linked with extra flags (-Wl,-u,vfprintf -lprintf_flt -lm)
// that Arduino sketches don't set by default. Without them, %f silently
// prints a literal '?' instead of the number -- confirmed by a real user
// whose board published {"temperature":?} to Azure. Doing the conversion
// by hand here avoids depending on any platform's printf float support at
// all, so it's correct on AVR (with or without the float-printf flags),
// ESP32, and anywhere else this library runs.
static int formatFloat2dp(char *buf, size_t bufCap, float value) {
    if (isnan(value) || isinf(value)) {
        return snprintf(buf, bufCap, "0.00"); // never feed a pathological value into the integer math below
    }
    bool negative = value < 0.0f;
    float absValue = negative ? -value : value;
    long scaled = (long)(absValue * 100.0f + 0.5f); // round to the nearest 0.01
    if (scaled == 0) negative = false; // avoid printing "-0.00" for tiny negatives that round to zero
    long intPart = scaled / 100;
    long fracPart = scaled % 100;
    return snprintf(buf, bufCap, "%s%ld.%02ld", negative ? "-" : "", intPart, fracPart);
}

// Appends `,"key":value` (or `"key":value` if first) to buf, tracking
// remaining space by hand -- fixed buffers only, no heap allocation, so a
// long-running board doesn't fragment its 6KB of SRAM over hours of use.
static bool appendField(char *buf, size_t bufCap, size_t *pos, bool first,
                         const char *key, float value) {
    char valueStr[24];
    int vn = formatFloat2dp(valueStr, sizeof(valueStr), value);
    if (vn <= 0 || (size_t)vn >= sizeof(valueStr)) return false;

    int n = snprintf(buf + *pos, bufCap - *pos, "%s\"%s\":%s",
                      first ? "" : ",", key, valueStr);
    if (n < 0 || (size_t)n >= bufCap - *pos) return false;
    *pos += (size_t)n;
    return true;
}

// Appends `s`, JSON-escaped, into buf starting at *pos -- handles the
// characters JSON actually requires escaping (", \, and control
// characters). Good enough for ordinary human-written status/test text;
// not a full Unicode-aware JSON encoder.
static void appendJsonEscaped(char *buf, size_t bufCap, size_t *pos, const char *s) {
    while (*s != '\0' && *pos + 1 < bufCap) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            if (*pos + 2 >= bufCap) break;
            buf[(*pos)++] = '\\';
            buf[(*pos)++] = (char)c;
        } else if (c == '\n' && *pos + 2 < bufCap) {
            buf[(*pos)++] = '\\'; buf[(*pos)++] = 'n';
        } else if (c == '\r' && *pos + 2 < bufCap) {
            buf[(*pos)++] = '\\'; buf[(*pos)++] = 'r';
        } else if (c == '\t' && *pos + 2 < bufCap) {
            buf[(*pos)++] = '\\'; buf[(*pos)++] = 't';
        } else if (c < 0x20) {
            if (*pos + 6 >= bufCap) break;
            int n = snprintf(buf + *pos, bufCap - *pos, "\\u%04x", c);
            if (n > 0) *pos += (size_t)n;
        } else {
            buf[(*pos)++] = (char)c;
        }
        s++;
    }
}

// Finds `"key":true` or `"key":false` in a JSON blob (Azure twin JSON never
// quotes booleans) and writes the result to *out. Same substring-search
// philosophy as dps_client.cpp's extractJsonString() -- not a full JSON
// parser, but reliable for the twin JSON shapes this library actually
// receives.
static bool extractJsonBool(const char *json, const char *key, bool *out) {
    char pattern[40];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return false;
    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p) return false;
    p++;
    while (*p == ' ') p++;
    if (strncmp(p, "true", 4) == 0) { *out = true; return true; }
    if (strncmp(p, "false", 5) == 0) { *out = false; return true; }
    return false;
}

// Same idea, for an unquoted JSON integer (used for twin "$version").
static bool extractJsonNumber(const char *json, const char *key, unsigned long *out) {
    char pattern[40];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return false;
    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p) return false;
    p++;
    while (*p == ' ') p++;
    char *endptr;
    unsigned long v = strtoul(p, &endptr, 10);
    if (endptr == p) return false;
    *out = v;
    return true;
}

// Reports a full IoT Plug-and-Play-style acknowledgment for one property, so
// IoT Central's dashboard shows "synced" rather than stuck "pending". `name`
// is one of the string literals passed to onBoolProperty() (short by
// construction, like publish()'s keys), so a fixed-size buffer here is safe
// -- snprintf's return value is still checked rather than assumed, in case a
// sketch ever registers an unusually long property name.
static void ackBoolProperty(const char *name, bool value, unsigned long version) {
    // 64, not 48: "$iothub/twin/PATCH/properties/reported/?$rid=" is 45
    // chars alone, and s_twinRequestId (unsigned long) can reach 10 digits
    // -- 48 left only 2 digits of headroom, silently truncating (and
    // therefore corrupting) $rid after just ~100 twin operations. Found via
    // a deliberate repro across the full unsigned long range, not by
    // re-reading the code -- snprintf bounds itself safely either way (no
    // memory-safety bug), but a truncated $rid is still wrong protocol
    // behavior, so the topic's own snprintf is now checked too, the same
    // way the payload's already was.
    char topic[64];
    int topicLen = snprintf(topic, sizeof(topic), "$iothub/twin/PATCH/properties/reported/?$rid=%lu", ++s_twinRequestId);
    if (topicLen <= 0 || (size_t)topicLen >= sizeof(topic)) {
        Serial.println("AzureIoT: internal error building ack topic -- dropped.");
        return;
    }

    char payload[160];
    int n = snprintf(payload, sizeof(payload),
                      "{\"%s\":{\"value\":%s,\"ac\":200,\"av\":%lu,\"ad\":\"completed\"}}",
                      name, value ? "true" : "false", version);
    if (n <= 0 || (size_t)n >= sizeof(payload)) {
        Serial.println("AzureIoT: property name too long to ack -- dropped.");
        return;
    }
    s_mqttClient.publish(topic, (const uint8_t *)payload, (unsigned int)n);
}

// Requests the full current twin once per connection, so a dashboard toggle
// flipped while the device was offline is picked up immediately on
// reconnect, rather than waiting for the next change. Response arrives
// asynchronously in mqttMessageCallback() below (topic "$iothub/twin/res/#").
static void requestFullTwin() {
    char topic[40]; // see ackBoolProperty() for why this needs real headroom, not just the "typical" $rid length
    int topicLen = snprintf(topic, sizeof(topic), "$iothub/twin/GET/?$rid=%lu", ++s_twinRequestId);
    if (topicLen <= 0 || (size_t)topicLen >= sizeof(topic)) {
        Serial.println("AzureIoT: internal error building twin GET topic -- dropped.");
        return;
    }
    s_mqttClient.publish(topic, (const uint8_t *)"", 0);
}

// PubSubClient's message callback. Handles two message shapes:
//  - "$iothub/twin/res/..."  -- response to our own requestFullTwin() GET
//    (full twin doc: {"desired":{...},"reported":{...}}), OR the empty ack
//    for our own ackBoolProperty() push (which we simply ignore).
//  - "$iothub/twin/PATCH/properties/desired/..." -- an incremental change
//    pushed live while connected (just the changed keys, e.g.
//    {"ledState":true,"$version":6}).
// Not a full JSON parser (same philosophy as dps_client.cpp): for the twin
// GET response, "desired" appears before "reported" in Azure's twin JSON, so
// searching from that point on reliably reaches the right section without
// needing real nested-object parsing -- confirmed against Azure's
// documented twin JSON shape, not just assumed.
static void mqttMessageCallback(char *topic, uint8_t *payload, unsigned int length) {
    if (s_boolPropCount == 0) return; // no sketch-registered properties to match against

    char body[300];
    if (length >= sizeof(body)) length = sizeof(body) - 1; // truncate rather than overflow
    memcpy(body, payload, length);
    body[length] = '\0';

    bool isTwinRes = strncmp(topic, "$iothub/twin/res/", 17) == 0;
    bool isDesiredPatch = strncmp(topic, "$iothub/twin/PATCH/properties/desired", 38) == 0;
    if (!isTwinRes && !isDesiredPatch) return;

    const char *searchIn = body;
    if (isTwinRes) {
        const char *desired = strstr(body, "\"desired\"");
        if (!desired) return; // an ack for our own reported-property push, not a GET response -- nothing to do
        searchIn = desired;
    }

    unsigned long version = 0;
    extractJsonNumber(searchIn, "$version", &version);

    for (size_t i = 0; i < s_boolPropCount; i++) {
        bool value;
        if (extractJsonBool(searchIn, s_boolProps[i].name, &value)) {
            s_boolProps[i].callback(value);
            ackBoolProperty(s_boolProps[i].name, value, version);
        }
    }
}

void AzureIoTClass::flush() {
    s_lastFlushMillis = millis();
    if (s_stagedCount == 0) return; // nothing staged since the last flush

    if (!ensureMqttConnected()) return;

    // 160, not 96: without declaring content-type/content-encoding, Azure
    // IoT Hub still ACCEPTS and forwards the message (so "last data
    // received" updates), but IoT Central specifically won't reliably
    // parse/display it as JSON without this -- confirmed against
    // Microsoft's own IoT Hub message-format docs, not assumed. The
    // declaration itself adds real length the old 96-byte buffer didn't
    // have room for.
    char topic[160];
    int topicLen = snprintf(topic, sizeof(topic), "devices/%s/messages/events/$.ct=application%%2Fjson&$.ce=utf-8", s_deviceId);
    if (topicLen <= 0 || (size_t)topicLen >= sizeof(topic)) {
        Serial.println("AzureIoT: internal error building telemetry topic -- dropped.");
        return;
    }

    char payload[220];
    const size_t cap = sizeof(payload);
    size_t pos = 0;
    if (pos < cap - 1) payload[pos++] = '{';
    bool first = true;
    for (size_t i = 0; i < s_stagedCount; i++) {
        if (!s_staged[i].set) continue;
        if (!appendField(payload, sizeof(payload), &pos, first, s_staged[i].key, s_staged[i].value)) {
            break; // ran out of room -- publish what fit rather than nothing
        }
        first = false;
        s_staged[i].set = false; // consumed
    }
    // Bounds-checked, not unconditional -- appendField() only bounds itself
    // against the capacity it's given; it doesn't know a closing '}' and a
    // NUL still need to follow it. An unconditional write here could run
    // past the buffer if staged keys were long enough to leave pos right at
    // the edge after the last successful append -- found the same pattern's
    // twin in publishText() via AddressSanitizer and checked here too,
    // where it turned out to be equally real (confirmed with the same tool
    // across a range of key lengths before this fix).
    if (pos < cap - 1) payload[pos++] = '}';
    payload[pos < cap ? pos : cap - 1] = '\0';

    if (s_mqttClient.publish(topic, (const uint8_t *)payload, (unsigned int)pos)) {
        Serial.print("Published: ");
        Serial.println(payload);
    } else {
        Serial.println("Publish failed.");
    }
}

void AzureIoTClass::publishText(const char *key, const char *value) {
    // Sends immediately -- does NOT go through the staged/batched path that
    // publish(key, float) uses. Deliberately kept separate: adding string
    // storage to the shared staging array would cost real RAM (a fixed-size
    // buffer per slot, whether or not any sketch ever calls this) on every
    // sketch using this library, including the many that never send text at
    // all. This is meant for occasional status messages and connectivity
    // testing, not high-frequency telemetry -- if you're calling this
    // rapidly in a tight loop, you'll bypass the send-rate bounding that
    // publish()/loop() give you for free.
    if (!ensureMqttConnected()) return;

    char topic[160]; // see flush() for why 160, not 96, and why the content-type/encoding declaration matters
    int topicLen = snprintf(topic, sizeof(topic), "devices/%s/messages/events/$.ct=application%%2Fjson&$.ce=utf-8", s_deviceId);
    if (topicLen <= 0 || (size_t)topicLen >= sizeof(topic)) {
        Serial.println("AzureIoT: internal error building telemetry topic -- dropped.");
        return;
    }

    char payload[220];
    const size_t cap = sizeof(payload);
    size_t pos = 0;

    // Every write below is individually bounds-checked against cap.
    // appendJsonEscaped() only bounds ITSELF against the capacity it's
    // given -- it has no idea what still needs to be written after it
    // returns, so the fixed characters around key/value (quotes, colon,
    // braces) can't just be written unconditionally afterward. An earlier
    // version did exactly that and could overflow this buffer if key+value
    // were long enough to fill it first -- caught with AddressSanitizer,
    // not by re-reading the code, so every write here is deliberately
    // defensive even where it looks redundant.
    if (pos < cap - 1) payload[pos++] = '{';
    if (pos < cap - 1) payload[pos++] = '"';
    appendJsonEscaped(payload, cap - 1, &pos, key);
    if (pos < cap - 1) payload[pos++] = '"';
    if (pos < cap - 1) payload[pos++] = ':';
    if (pos < cap - 1) payload[pos++] = '"';
    appendJsonEscaped(payload, cap - 1, &pos, value);
    if (pos < cap - 1) payload[pos++] = '"';
    if (pos < cap - 1) payload[pos++] = '}';
    payload[pos < cap ? pos : cap - 1] = '\0';

    if (s_mqttClient.publish(topic, (const uint8_t *)payload, (unsigned int)pos)) {
        Serial.print("Published: ");
        Serial.println(payload);
    } else {
        Serial.println("Publish failed.");
    }
}

void AzureIoTClass::begin(const char *wifiSsid, const char *wifiPassword,
                           const char *idScope, const char *deviceId, const char *deviceKey,
                           const char *modelId) {
    strncpy(s_wifiSsid, wifiSsid, sizeof(s_wifiSsid) - 1);
    strncpy(s_wifiPassword, wifiPassword, sizeof(s_wifiPassword) - 1);
    strncpy(s_idScope, idScope, sizeof(s_idScope) - 1);
    strncpy(s_deviceIdInput, deviceId, sizeof(s_deviceIdInput) - 1);
    strncpy(s_deviceKey, deviceKey, sizeof(s_deviceKey) - 1);
    if (modelId) {
        strncpy(s_modelId, modelId, sizeof(s_modelId) - 1);
    }

    connectWiFi();

    // Configure TLS trust for the persistent MQTT client once -- a no-op on
    // WiFiNINA boards, sets the embedded root CA on ESP32. Doing this once
    // here (not on every reconnect) matches the same reasoning as
    // setBufferSize() below.
    configureSecureClient(s_wifiClient);

    // Registered once, unconditionally -- cheap (a function pointer, no
    // allocation) even for sketches that never call onBoolProperty(), so
    // there's no reason to gate this behind s_boolPropCount like the
    // subscribe/twin-GET calls in ensureMqttConnected() are.
    s_mqttClient.setCallback(mqttMessageCallback);

    // Retry provisioning with exponential backoff instead of halting
    // forever on the first failure -- a transient Azure-side hiccup or a
    // DHCP/DNS blip during boot shouldn't brick the board until someone
    // finds it and power-cycles it. Doubles each time, capped at
    // s_provisionRetryMaxMs, and retries indefinitely -- even a permanent
    // misconfiguration (wrong ID scope/key) just settles into a slow,
    // harmless retry cadence rather than spamming Azure or wedging the
    // device.
    unsigned long backoffMs = s_provisionRetryInitialMs;
    while (!provisionDevice()) {
        Serial.print("Provisioning failed -- retrying in ");
        Serial.print(backoffMs / 1000);
        Serial.println("s. Check the ID scope/device ID/device key passed to AzureIoT.begin() if this keeps happening.");

        // Check Wi-Fi on every tick, not just once after the full wait --
        // otherwise a drop early in a long (up to 15 min, at the backoff
        // cap) wait wouldn't get noticed, let alone escalated through
        // connectWiFi()'s 1-min/5-min remedies, until the whole wait
        // elapsed. Break out the moment it's noticed instead.
        unsigned long waitStart = millis();
        while (millis() - waitStart < backoffMs) {
            delay(5000);
            Serial.print(".");
            if (WiFi.status() != WL_CONNECTED) break;
        }
        Serial.println();

        if (WiFi.status() != WL_CONNECTED) {
            connectWiFi(); // has its own reinit/reset escalation if this drags on
        }

        backoffMs *= 2;
        if (backoffMs > s_provisionRetryMaxMs) backoffMs = s_provisionRetryMaxMs;
    }

    // Allocate the MQTT buffer exactly once, for the sketch's entire
    // lifetime -- see the note in ensureMqttConnected() for why this must
    // not be called again on every reconnect. If this fails, the board has
    // essentially no usable heap left; halt visibly rather than limp along
    // with a broken MQTT client that will fail every publish silently.
    if (!s_mqttClient.setBufferSize(1024)) {
        Serial.println("Halting: could not allocate MQTT buffer (out of memory).");
        while (true) { delay(1000); }
    }

    ensureMqttConnected();
    s_lastFlushMillis = millis();
}

void AzureIoTClass::loop() {
    if (s_watchdogEnabled) platformWatchdogPet();

    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }
    ensureMqttConnected();
    s_mqttClient.loop();
    retryPendingReports(); // catch up on anything reportBoolProperty() couldn't send earlier

    if (millis() - s_lastFlushMillis >= s_sendIntervalMs) {
        flush();
    }
}

void AzureIoTClass::publish(const char *key, float value) {
    // Overwrite if this key is already staged; otherwise add a new slot.
    for (size_t i = 0; i < s_stagedCount; i++) {
        if (strcmp(s_staged[i].key, key) == 0) {
            s_staged[i].value = value;
            s_staged[i].set = true;
            return;
        }
    }
    if (s_stagedCount < 16) {
        s_staged[s_stagedCount].key = key;
        s_staged[s_stagedCount].value = value;
        s_staged[s_stagedCount].set = true;
        s_stagedCount++;
    } else {
        Serial.println("AzureIoT.publish: too many distinct keys staged (max 16) -- dropped.");
    }
}

void AzureIoTClass::sendNow() {
    flush();
}

void AzureIoTClass::onBoolProperty(const char *name, BoolPropertyCallback callback) {
    if (s_boolPropCount < 16) {
        s_boolProps[s_boolPropCount].name = name;
        s_boolProps[s_boolPropCount].callback = callback;
        s_boolPropCount++;
    } else {
        Serial.println("AzureIoT.onBoolProperty: too many properties registered (max 16) -- dropped.");
    }
}

void AzureIoTClass::enableWatchdog() {
    platformWatchdogEnable();
    s_watchdogEnabled = true;
    Serial.println("Watchdog enabled -- device resets if not petted for ~8s.");
}

// Shared by reportBoolProperty() and retryPendingReports() below -- builds
// and sends the flat {"name":value} reported-property update, returning
// whether it actually went out. Doesn't touch s_pendingReports itself;
// callers decide what staging/clearing to do with the result.
static bool sendReportedProperty(const char *name, bool value) {
    if (!s_mqttClient.connected()) return false;

    char topic[64]; // see ackBoolProperty() for why this needs real headroom, not just the "typical" $rid length
    int topicLen = snprintf(topic, sizeof(topic), "$iothub/twin/PATCH/properties/reported/?$rid=%lu", ++s_twinRequestId);
    if (topicLen <= 0 || (size_t)topicLen >= sizeof(topic)) {
        Serial.println("AzureIoT: internal error building report topic -- dropped.");
        return false;
    }

    char payload[64];
    int n = snprintf(payload, sizeof(payload), "{\"%s\":%s}", name, value ? "true" : "false");
    if (n <= 0 || (size_t)n >= sizeof(payload)) {
        Serial.println("AzureIoT.reportBoolProperty: property name too long -- dropped.");
        return false;
    }

    return s_mqttClient.publish(topic, (const uint8_t *)payload, (unsigned int)n);
}

// Retries anything reportBoolProperty() couldn't send immediately (MQTT was
// down, or the publish() call itself failed). Called from loop() every
// iteration once MQTT is connected -- cheap when there's nothing pending
// (a fixed 16-slot scan, no allocation), and stops paying that cost the
// moment every slot is caught up.
static void retryPendingReports() {
    for (size_t i = 0; i < s_pendingReportCount; i++) {
        if (!s_pendingReports[i].pending) continue;
        if (sendReportedProperty(s_pendingReports[i].name, s_pendingReports[i].value)) {
            s_pendingReports[i].pending = false;
        }
        // else: still not connected, or this particular publish() failed --
        // leave pending=true, try again next loop().
    }
}

void AzureIoTClass::reportBoolProperty(const char *name, bool value) {
    // Flat form -- {"name":value} -- no ac/av/ad wrapper, since that
    // convention specifically means "acknowledging a particular desired
    // version" (see ackBoolProperty() above), which doesn't apply to a
    // device-initiated change nothing from the cloud asked for.

    // Stage first (overwrite if this name is already pending), THEN attempt
    // an immediate send -- so a failed/skipped send still leaves a correct
    // record behind for retryPendingReports() to pick up later, instead of
    // the value only existing transiently on the stack.
    size_t slot = s_pendingReportCount; // index to use, found or newly allocated below
    bool found = false;
    for (size_t i = 0; i < s_pendingReportCount; i++) {
        if (strcmp(s_pendingReports[i].name, name) == 0) {
            slot = i;
            found = true;
            break;
        }
    }
    if (!found) {
        if (s_pendingReportCount >= 16) {
            Serial.println("AzureIoT.reportBoolProperty: too many distinct property names pending (max 16) -- dropped.");
            return;
        }
        s_pendingReportCount++;
    }
    s_pendingReports[slot].name = name;
    s_pendingReports[slot].value = value;
    s_pendingReports[slot].pending = true;

    if (sendReportedProperty(name, value)) {
        s_pendingReports[slot].pending = false;
    }
    // else: left pending -- retryPendingReports() (called from loop()) will
    // catch it once MQTT reconnects, rather than the change being lost
    // outright the way it would have been before this retry buffer existed.
}

void AzureIoTClass::setDpsGlobalHost(const char *host) {
    strncpy(s_dpsGlobalHost, host, sizeof(s_dpsGlobalHost) - 1);
}
void AzureIoTClass::setSasTokenLifetime(unsigned long seconds) { s_sasTokenLifetimeSecs = seconds; }
void AzureIoTClass::setSendInterval(unsigned long ms) { s_sendIntervalMs = ms; }
void AzureIoTClass::setWifiConnectTimeout(unsigned long ms) { s_wifiConnectTimeoutMs = ms; }
void AzureIoTClass::setMqttReconnectCooldown(unsigned long ms) { s_mqttReconnectCooldownMs = ms; }
void AzureIoTClass::setWifiHardReinitAfter(unsigned long ms) { s_wifiHardReinitAfterMs = ms; }
void AzureIoTClass::setWifiForceResetAfter(unsigned long ms) { s_wifiForceResetAfterMs = ms; }
void AzureIoTClass::setProvisionRetryInitial(unsigned long ms) { s_provisionRetryInitialMs = ms; }
void AzureIoTClass::setProvisionRetryMax(unsigned long ms) { s_provisionRetryMaxMs = ms; }

AzureIoTClass AzureIoT;
