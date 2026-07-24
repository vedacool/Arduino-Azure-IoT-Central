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

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_LED = 2;   // ESP32: onboard LED. GPIO 7 is a flash pin on ESP32 -- never usable
#else
const int PIN_LED = 7;   // Uno WiFi Rev2 + Grove Base Shield: Grove D7
#endif

void onLedState(bool on) {
    digitalWrite(PIN_LED, on ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);

    // Must be called BEFORE begin() -- see AzureIoT.h for the full design.
    AzureIoT.onBoolProperty("ledState", onLedState);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND delivering property updates to onLedState() above
}
