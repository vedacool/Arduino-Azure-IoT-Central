// Exercise 4 -- Touch Sensor
//
// Grove Touch Sensor on digital pin 3, connected to Azure IoT Central via
// the AzureIoT library.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_TOUCH = 3;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_TOUCH, INPUT);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int touchSense = digitalRead(PIN_TOUCH);
    AzureIoT.publish("touch", (float)touchSense);

    delay(100); // touch is a fast-changing input, so poll it a bit quicker
}
