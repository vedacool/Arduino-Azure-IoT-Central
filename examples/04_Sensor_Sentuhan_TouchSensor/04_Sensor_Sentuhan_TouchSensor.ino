// Module 4, Exercise 4 -- Sensor Sentuhan (Touch Sensor)
//
// Grove Touch Sensor on digital pin 3, connected to Azure IoT Central via
// the AzureIoT library.
//
// Setup: copy config.h.example to config.h in this folder and fill in your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_TOUCH = 3;

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    pinMode(PIN_TOUCH, INPUT);
    AzureIoT.begin();
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int touchSense = digitalRead(PIN_TOUCH);
    AzureIoT.publish("touch", (float)touchSense);

    delay(100); // touch is a fast-changing input, so poll it a bit quicker
}
