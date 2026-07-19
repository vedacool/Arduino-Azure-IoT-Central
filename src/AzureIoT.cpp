#include "AzureIoT.h"
#include "platform.h"
#include <PubSubClient.h>
#include <string.h>
#include "config.h"
#include "sas_token.h"
#include "dps_client.h"
#include "reset.h"

static SecureWiFiClient s_wifiClient;
static PubSubClient s_mqttClient(s_wifiClient);

static char s_iotHubHost[128] = {0};
static char s_deviceId[64] = {0};
static char s_sasToken[300] = {0};
static unsigned long s_sasTokenExpiry = 0;

static unsigned long s_lastFlushMillis = 0;
static unsigned long s_lastMqttAttemptMillis = 0;

// Staged telemetry -- publish() writes here, flush() reads it and clears it.
struct StagedValue { const char *key; float value; bool set; };
static StagedValue s_staged[16];
static size_t s_stagedCount = 0;

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
        Serial.println(WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        unsigned long start = millis();
        bool restartAttempt = false;
        while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
            delay(500);
            Serial.print(".");

            if (millis() - downSince > WIFI_FORCE_RESET_AFTER_MS) {
                Serial.println();
                Serial.println("WiFi still down after the reset threshold -- forcing a full device reset.");
                forceReset();
            }
            if (!didHardReinit && millis() - downSince > WIFI_HARD_REINIT_AFTER_MS) {
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
        Serial.println("WiFi connect timed out -- check WIFI_SSID/WIFI_PASSWORD in config.h. Retrying...");
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
    while (platformGetUnixTime() < 1700000000UL) { // sanity floor: after Nov 2023
        Serial.print(".");
        delay(500);
    }
    Serial.println();
}

static bool provisionDevice() {
    Serial.println("Provisioning via DPS...");
    DpsResult result;
    if (!dpsProvision(IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY, result)) {
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
    char resourceUri[192];
    snprintf(resourceUri, sizeof(resourceUri), "%s/devices/%s", s_iotHubHost, s_deviceId);
    unsigned long expiry = now + SAS_TOKEN_LIFETIME_SECS;
    if (build_sas_token(IOTC_DEVICE_KEY, resourceUri, expiry, s_sasToken, sizeof(s_sasToken)) == 0) {
        Serial.println("Failed to build SAS token.");
        return false;
    }
    s_sasTokenExpiry = expiry;
    return true;
}

bool AzureIoTClass::ensureMqttConnected() {
    if (s_mqttClient.connected()) return true;
    if (millis() - s_lastMqttAttemptMillis < MQTT_RECONNECT_COOLDOWN_MS) return false;
    s_lastMqttAttemptMillis = millis();

    if (!refreshSasTokenIfNeeded()) return false;

    s_mqttClient.setServer(s_iotHubHost, 8883);
    // NOTE: setBufferSize() is called ONCE in begin(), not here. PubSubClient's
    // setBufferSize() does a heap malloc()/realloc() -- calling it on every
    // reconnect (which will happen periodically over weeks of normal Wi-Fi
    // blips) would mean needless repeated heap churn on a 6KB-RAM board for
    // no benefit, since the size never changes.

    char username[220];
    snprintf(username, sizeof(username), "%s/%s/?api-version=2021-04-12",
             s_iotHubHost, s_deviceId);

    Serial.print("Connecting to MQTT as ");
    Serial.println(s_deviceId);
    if (s_mqttClient.connect(s_deviceId, username, s_sasToken)) {
        Serial.println("MQTT connected.");
        return true;
    }
    Serial.print("MQTT connect failed, rc=");
    Serial.println(s_mqttClient.state());
    return false;
}

// Appends `,"key":value` (or `"key":value` if first) to buf, tracking
// remaining space by hand -- fixed buffers only, no heap allocation, so a
// long-running board doesn't fragment its 6KB of SRAM over hours of use.
static bool appendField(char *buf, size_t bufCap, size_t *pos, bool first,
                         const char *key, float value) {
    int n = snprintf(buf + *pos, bufCap - *pos, "%s\"%s\":%.2f",
                      first ? "" : ",", key, value);
    if (n < 0 || (size_t)n >= bufCap - *pos) return false;
    *pos += (size_t)n;
    return true;
}

void AzureIoTClass::flush() {
    s_lastFlushMillis = millis();
    if (s_stagedCount == 0) return; // nothing staged since the last flush

    if (!ensureMqttConnected()) return;

    char topic[96];
    snprintf(topic, sizeof(topic), "devices/%s/messages/events/", s_deviceId);

    char payload[220];
    size_t pos = 0;
    payload[pos++] = '{';
    bool first = true;
    for (size_t i = 0; i < s_stagedCount; i++) {
        if (!s_staged[i].set) continue;
        if (!appendField(payload, sizeof(payload), &pos, first, s_staged[i].key, s_staged[i].value)) {
            break; // ran out of room -- publish what fit rather than nothing
        }
        first = false;
        s_staged[i].set = false; // consumed
    }
    payload[pos++] = '}';
    payload[pos] = '\0';

    if (s_mqttClient.publish(topic, (const uint8_t *)payload, (unsigned int)pos)) {
        Serial.print("Published: ");
        Serial.println(payload);
    } else {
        Serial.println("Publish failed.");
    }
}

void AzureIoTClass::begin() {
    connectWiFi();

    // Configure TLS trust for the persistent MQTT client once -- a no-op on
    // WiFiNINA boards, sets the embedded root CA on ESP32. Doing this once
    // here (not on every reconnect) matches the same reasoning as
    // setBufferSize() below.
    configureSecureClient(s_wifiClient);

    // Retry provisioning with exponential backoff instead of halting
    // forever on the first failure -- a transient Azure-side hiccup or a
    // DHCP/DNS blip during boot shouldn't brick the board until someone
    // finds it and power-cycles it. Doubles each time, capped at
    // PROVISION_RETRY_MAX_MS, and retries indefinitely -- even a permanent
    // misconfiguration (wrong ID scope/key) just settles into a slow,
    // harmless retry cadence rather than spamming Azure or wedging the
    // device.
    unsigned long backoffMs = PROVISION_RETRY_INITIAL_MS;
    while (!provisionDevice()) {
        Serial.print("Provisioning failed -- retrying in ");
        Serial.print(backoffMs / 1000);
        Serial.println("s. Check IOTC_ID_SCOPE/IOTC_DEVICE_ID/IOTC_DEVICE_KEY in config.h if this keeps happening.");

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
        if (backoffMs > PROVISION_RETRY_MAX_MS) backoffMs = PROVISION_RETRY_MAX_MS;
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
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }
    ensureMqttConnected();
    s_mqttClient.loop();

    if (millis() - s_lastFlushMillis >= SEND_INTERVAL_MS) {
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

AzureIoTClass AzureIoT;
