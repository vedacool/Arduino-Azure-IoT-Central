// Example 21 -- Remote Temperature Alarm
//
// Sounds a buzzer (digital pin 6) when ANOTHER device's "temperature"
// telemetry crosses 30 degrees C -- e.g. Device A is one of the
// temperature-sensor examples publishing its own readings, and THIS
// device (Device B) watches Device A's readings and reacts to them,
// without ever running its own temperature sensor.
//
// This is fundamentally different from every other example in this set:
// it doesn't use this device's own MQTT connection at all for the watching
// part -- it POLLS Azure's REST API on a timer, because devices cannot
// subscribe to another device's telemetry directly (a deliberate Azure
// security boundary, not a limitation of this library -- see the README).
//
// Setup, in addition to the usual Wi-Fi + Azure credentials:
// 1. Generate an IoT Central "API token" -- NOT the same as this device's
//    own Connect credentials. In IoT Central:
//    Permissions > API tokens > New > role "App Operator" (the
//    least-privileged role that can still read telemetry -- don't use
//    App Administrator or App Builder for this).
// 2. Edit config.h in this folder: your Wi-Fi + Azure credentials as
//    usual, PLUS IOTC_REMOTE_APP_SUBDOMAIN (your app's subdomain, e.g.
//    "myapp" from "myapp.azureiotcentral.com") and IOTC_REMOTE_API_TOKEN
//    (the full token string, including "SharedAccessSignature ...").
// 3. Change REMOTE_DEVICE_ID below to the actual device ID of whichever
//    device is publishing the "temperature" you want to watch.

#include <AzureIoT.h>
#include "config.h"

const int PIN_BUZZER = 6;
const char *REMOTE_DEVICE_ID = "arduino1"; // change this to the device you want to watch
const float ALARM_THRESHOLD_C = 30.0f;

void onRemoteTemperature(float value) {
    Serial.print("Remote temperature: ");
    Serial.println(value);

    if (value > ALARM_THRESHOLD_C) {
        tone(PIN_BUZZER, 1000);
    } else {
        noTone(PIN_BUZZER);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUZZER, OUTPUT);

    // Must be called BEFORE begin() -- see AzureIoT.h for the full design.
    AzureIoT.setRemoteAccess(IOTC_REMOTE_APP_SUBDOMAIN, IOTC_REMOTE_API_TOKEN);
    AzureIoT.onRemoteTelemetry("temperature", REMOTE_DEVICE_ID, onRemoteTemperature);

    // Optional: how often to poll for the remote value (default 15000ms).
    // Fastest allowed is 1000ms -- see AzureIoT.h for why going faster than
    // that risks Azure's own shared, per-application rate limit, not just
    // this device's resources.
    // AzureIoT.setRemotePollInterval(15000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND polling for the remote telemetry value on the interval above
}
