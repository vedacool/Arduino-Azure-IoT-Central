// Example 3 -- Water Sensor
//
// Grove Water Sensor on digital pin 5, connected to Azure IoT Central via
// the AzureIoT library. Publishes 1 when water is detected, 0 otherwise.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_WATER = 5;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_WATER, INPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int wet = digitalRead(PIN_WATER);
    AzureIoT.publish("water", (float)wet);

    delay(500);
}
