// Example 16 -- Buzzer Cloud Control
//
// Grove Buzzer on digital pin 6, controlled from the Azure IoT Central
// dashboard using a writable property, same pattern as
// 15_LedCloudControl -- proof the onBoolProperty() API isn't LED-specific.
//
// Add a boolean writable property named "buzzerOn" to your device
// template in IoT Central for the toggle to appear in the dashboard.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_BUZZER = 4;   // ESP32: safe GPIO. GPIO 6 is a flash pin on ESP32 -- never usable
#else
const int PIN_BUZZER = 6;   // Uno WiFi Rev2 + Grove Base Shield: Grove D6
#endif

void onBuzzerOn(bool on) {
    if (on) {
        tone(PIN_BUZZER, 1000);
    } else {
        noTone(PIN_BUZZER);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUZZER, OUTPUT);

    // Must be called BEFORE begin() -- see AzureIoT.h for the full design.
    AzureIoT.onBoolProperty("buzzerOn", onBuzzerOn);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND delivering property updates to onBuzzerOn() above
}
