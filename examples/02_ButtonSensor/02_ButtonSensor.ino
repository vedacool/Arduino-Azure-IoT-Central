// Example 2 -- Button
//
// Grove Button (or a plain pushbutton) on digital pin 4, connected to
// Azure IoT Central via the AzureIoT library. Publishes 1 while pressed,
// 0 while released.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_BUTTON = 4;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUTTON, INPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int pressed = digitalRead(PIN_BUTTON);
    AzureIoT.publish("button", (float)pressed);

    delay(100); // a button is a fast-changing input, so poll it a bit quicker
}
