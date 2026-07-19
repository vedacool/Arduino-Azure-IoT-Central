#include "AzureIoT.h"
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "config.h"
#include "sas_token.h"
#include "dps_client.h"

static WiFiSSLClient s_wifiClient;
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
    while (true) {
        Serial.print("Connecting to WiFi: ");
        Serial.println(WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) break;

        Serial.println();
        Serial.println("WiFi connect timed out -- check WIFI_SSID/WIFI_PASSWORD in config.h. Retrying...");
    }
    Serial.println();
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());

    // Wait for the NINA module's NTP-synced clock -- SAS tokens are time-based
    // and Azure rejects tokens whose expiry looks wrong, so we must not sign
    // anything before the clock is real.
    Serial.print("Waiting for network time");
    while (WiFi.getTime() < 1700000000UL) { // sanity floor: after Nov 2023
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
    unsigned long now = WiFi.getTime();
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
    s_mqttClient.setBufferSize(1024); // PubSubClient >=2.8; no library edits needed

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
    if (!provisionDevice()) {
        Serial.println("Halting: provisioning failed. Check ID scope / device ID / key.");
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
