// Example 11 -- LED Status Report
//
// Grove LED on digital pin 7, blinking on a local timer. This example does
// NOT accept cloud control (see 15_LedCloudControl for that) -- it just
// reports its own on/off state to Azure IoT Central as ordinary telemetry,
// the same way a sensor reading would be published. A good first step
// before adding two-way control: prove the device can tell the cloud what
// an actuator is doing.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_LED = 7;
const unsigned long BLINK_INTERVAL_MS = 1000;

bool ledState = false;
unsigned long lastBlinkMillis = 0;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    if (millis() - lastBlinkMillis >= BLINK_INTERVAL_MS) {
        lastBlinkMillis = millis();
        ledState = !ledState;
        digitalWrite(PIN_LED, ledState ? HIGH : LOW);
        AzureIoT.publish("ledState", ledState ? 1.0f : 0.0f);
    }
}
