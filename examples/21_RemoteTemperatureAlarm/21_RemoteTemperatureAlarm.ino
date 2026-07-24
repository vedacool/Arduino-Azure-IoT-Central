// Example 21 -- Remote Temperature Alarm
//
// Sounds a buzzer (digital pin 6) when ANOTHER device's "temperature"
// telemetry crosses 30 degrees C -- e.g. Device A is one of the
// temperature-sensor examples publishing its own readings, and THIS
// device (Device B) watches Device A's readings and reacts to them,
// without ever running its own temperature sensor.
//
// This device runs in PULL-ONLY MODE: it connects to Wi-Fi and polls
// Azure's REST API, but does NOT connect its own identity to Azure at all
// (no DPS, no MQTT) -- because onRemoteTelemetry() is registered below,
// AzureIoT.begin() automatically skips that entirely. A device can, for
// now, only ever SEND its own telemetry OR PULL another device's telemetry
// -- not both at once (real hardware testing found running a persistent
// MQTT connection and a separate periodic HTTPS poll at the same time is a
// known-fragile combination on this board's Wi-Fi hardware -- see
// DEVELOPMENT.md). This means the IOTC_ID_SCOPE/IOTC_DEVICE_ID/
// IOTC_DEVICE_KEY fields in config.h are simply UNUSED here -- this device
// never provisions an identity of its own.
//
// This is fundamentally different from every other example in this set in
// another way too: it doesn't use MQTT at all -- it POLLS Azure's REST API
// on a timer, because devices cannot subscribe to another device's
// telemetry directly (a deliberate Azure security boundary, not a
// limitation of this library -- see the README).
//
// Setup:
// 1. Generate an IoT Central "API token" -- NOT the same as a device's own
//    Connect credentials. In IoT Central:
//    Permissions > API tokens > New > role "App Operator" (the
//    least-privileged role that can still read telemetry -- don't use
//    App Administrator or App Builder for this).
// 2. Edit config.h in this folder: your Wi-Fi credentials, PLUS
//    IOTC_REMOTE_APP_SUBDOMAIN (your app's subdomain, e.g. "myapp" from
//    "myapp.azureiotcentral.com") and IOTC_REMOTE_API_TOKEN (the full
//    token string, including "SharedAccessSignature ...").
// 3. Change REMOTE_DEVICE_ID below to the actual device ID of whichever
//    device is publishing the "temperature" you want to watch.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_BUZZER = 4;   // ESP32: safe GPIO. GPIO 6 is a flash pin on ESP32 -- never usable
#else
const int PIN_BUZZER = 6;   // Uno WiFi Rev2 + Grove Base Shield: Grove D6
#endif
const char *REMOTE_DEVICE_ID = "arduino1"; // change this to the device you want to watch
const float ALARM_ON_C  = 30.0f; // buzzer turns ON above this...
const float ALARM_OFF_C = 29.0f; // ...and OFF below this. The dead band between
                                 // the two stops a value hovering around 30 from
                                 // chattering the buzzer on/off between polls --
                                 // the same hysteresis idea as examples 18 and 19.

bool alarmOn = false;

void onRemoteTemperature(float value) {
    Serial.print("Remote temperature: ");
    Serial.println(value);

    if (!alarmOn && value > ALARM_ON_C) {
        alarmOn = true;
        tone(PIN_BUZZER, 1000);
    } else if (alarmOn && value < ALARM_OFF_C) {
        alarmOn = false;
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

    // Because onRemoteTelemetry() was already called above, this runs in
    // PULL-ONLY MODE -- only WIFI_SSID/WIFI_PASSWORD actually matter here.
    // IOTC_ID_SCOPE/IOTC_DEVICE_ID/IOTC_DEVICE_KEY are unused (this device
    // never connects its own identity to Azure) -- config.h's placeholder
    // values for those three are fine to leave exactly as they are.
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects
                      // AND polling for the remote telemetry value on the interval above
}
