// Example 15 -- LED Cloud Control
//
// Grove LED on digital pin 7, controlled directly from the Azure IoT
// Central dashboard using a writable property. Flip the "ledState" toggle
// in IoT Central's device view and the LED responds -- no local logic
// decides when it turns on, the cloud does.
//
// Add a boolean writable property named "ledState" to your device
// template in IoT Central (Devices > your device's template > Add
// capability > Property > Writable) for the toggle to appear in the
// dashboard.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_LED = 7;

void onLedState(bool on) {
    digitalWrite(PIN_LED, on ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);

    // Must be called BEFORE begin() -- see AzureIoT.h for the full design.
    AzureIoT.onBoolProperty("ledState", onLedState);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY, IOTC_MODEL_ID);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND delivering property updates to onLedState() above
}
