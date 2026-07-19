// Module 4, Exercise 5 -- Sensor Kekeruhan (Turbidity Sensor)
//
// Grove Turbidity Sensor on analog pin A3, connected to Azure IoT Central
// via the AzureIoT library.
//
// Setup: copy config.h.example to config.h in this folder and fill in your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_TURBIDITY = A3;

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    AzureIoT.begin();
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    // Same math as the original Module 2/4 tutorials.
    int sensorValue = analogRead(PIN_TURBIDITY);
    float turbidity = sensorValue * (5.0f / 1024.0f);

    AzureIoT.publish("turbidity", turbidity);

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
