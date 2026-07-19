// uno_wifi_azure.ino
// Arduino Uno WiFi Rev2 -> Azure IoT Central, without any PC-side tooling.
//
// Replaces the firedog1024/arduino-uno-wifi-iotc + Azure/dps-keygen combo:
//  - dps-keygen is archived/dead (its hard-coded-connection-string feature was
//    switched off by Microsoft in Jan 2020) -- this sketch does DPS
//    registration ON THE DEVICE instead, so there is no PC tool step and no
//    stale hostname if IoT Central reassigns the device to a different hub.
//  - SHA-256 / HMAC / base64 are verified against Python's hashlib/hmac
//    (see /test in the accompanying project) rather than hand-rolled and
//    hoped-for.
//  - MQTT buffer is grown via PubSubClient::setBufferSize() (available since
//    PubSubClient v2.8) instead of hand-editing the library's header file.
//  - No String concatenation in the hot path -- fixed char buffers only, to
//    avoid heap fragmentation on a board with 6KB of SRAM.
//
// REQUIRED ONE-TIME MANUAL STEP (cannot be done from the sketch):
//   Tools > WiFiNINA Firmware/Certificates Updater > add the DPS and IoT Hub
//   domains so the NINA module trusts Azure's current TLS chain
//   (DigiCert Global Root G2, migration completed Sept 2024):
//     - global.azure-devices-provisioning.net
//     - <your-app>.azure-devices.net (or add after first provisioning once
//       you know the assigned hub name)

#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "config.h"
#include "sas_token.h"
#include "dps_client.h"

WiFiSSLClient wifiClient;
PubSubClient mqttClient(wifiClient);

char g_iotHubHost[128] = {0};
char g_deviceId[64] = {0};
char g_sasToken[300] = {0};
unsigned long g_sasTokenExpiry = 0;

unsigned long g_lastSensorReadMillis = 0;

// ---- sensor state (Grove peripherals from Modules 2 & 4) ----
const int PIN_TEMP = A0;
float g_temperature = 0.0f;

static void connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
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
    strncpy(g_iotHubHost, result.assignedHub, sizeof(g_iotHubHost) - 1);
    strncpy(g_deviceId, result.deviceId, sizeof(g_deviceId) - 1);
    Serial.print("Assigned hub: ");
    Serial.println(g_iotHubHost);
    return true;
}

static bool refreshSasTokenIfNeeded() {
    unsigned long now = WiFi.getTime();
    if (g_sasToken[0] != '\0' && now < g_sasTokenExpiry - 60) {
        return true; // still valid for at least another minute
    }
    char resourceUri[192];
    snprintf(resourceUri, sizeof(resourceUri), "%s/devices/%s", g_iotHubHost, g_deviceId);
    unsigned long expiry = now + SAS_TOKEN_LIFETIME_SECS;
    if (build_sas_token(IOTC_DEVICE_KEY, resourceUri, expiry, g_sasToken, sizeof(g_sasToken)) == 0) {
        Serial.println("Failed to build SAS token.");
        return false;
    }
    g_sasTokenExpiry = expiry;
    return true;
}

static bool connectMqtt() {
    if (!refreshSasTokenIfNeeded()) return false;

    mqttClient.setServer(g_iotHubHost, 8883);
    mqttClient.setBufferSize(1024); // PubSubClient >=2.8; no library edits needed

    char username[220];
    snprintf(username, sizeof(username), "%s/%s/?api-version=2021-04-12",
              g_iotHubHost, g_deviceId);

    Serial.print("Connecting to MQTT as ");
    Serial.println(g_deviceId);
    if (mqttClient.connect(g_deviceId, username, g_sasToken)) {
        Serial.println("MQTT connected.");
        return true;
    }
    Serial.print("MQTT connect failed, rc=");
    Serial.println(mqttClient.state());
    return false;
}

static void readSensors() {
    // Thermistor on A0, same math as Module 4's temperature sensor code,
    // kept because it's already correct -- just moved out of String-based
    // payload building.
    const int B = 4275;
    const long R0 = 100000;
    int a = analogRead(PIN_TEMP);
    float R = 1023.0f / (float)a - 1.0f;
    R = (float)R0 * R;
    g_temperature = 1.0f / (log(R / (float)R0) / B + 1.0f / 298.15f) - 273.15f;
}

static void publishTelemetry() {
    char topic[96];
    snprintf(topic, sizeof(topic), "devices/%s/messages/events/", g_deviceId);

    char payload[96];
    // Fixed-precision snprintf instead of String concatenation/replace --
    // smaller, faster, and doesn't fragment the heap over a multi-hour run.
    int n = snprintf(payload, sizeof(payload), "{\"temperature\":%.2f}", g_temperature);

    if (mqttClient.publish(topic, (const uint8_t *)payload, (unsigned int)n)) {
        Serial.print("Published: ");
        Serial.println(payload);
    } else {
        Serial.println("Publish failed.");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }

    connectWiFi();

    if (!provisionDevice()) {
        Serial.println("Halting: provisioning failed. Check ID scope / device ID / key.");
        while (true) { delay(1000); }
    }

    connectMqtt();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (!mqttClient.connected()) {
        connectMqtt();
    }
    mqttClient.loop();

    if (millis() - g_lastSensorReadMillis > SENSOR_READ_INTERVAL_MS) {
        g_lastSensorReadMillis = millis();
        readSensors();
        publishTelemetry();
    }
}
