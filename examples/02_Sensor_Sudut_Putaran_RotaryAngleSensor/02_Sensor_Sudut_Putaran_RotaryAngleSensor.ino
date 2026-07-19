// Module 4, Exercise 2 -- Sensor Sudut Putaran (Rotary Angle Sensor)
//
// Grove Rotary Angle Sensor on analog pin A1, connected to Azure IoT
// Central via the AzureIoT library.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_ROTARY_ANGLE = A1;
const float ADC_REF = 5.0f;     // 3.3 instead if your board's Vcc switch is set to 3V3
const float FULL_ANGLE = 300.0f; // the sensor's mechanical range in degrees
const float GROVE_VCC = 5.0f;

void setup() {
    Serial.begin(115200);
    AzureIoT.begin();
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    // Same math as the original Module 2/4 tutorials.
    int sensorValue = analogRead(PIN_ROTARY_ANGLE);
    float voltage = (float)sensorValue * ADC_REF / 1023.0f;
    float rotaryAngle = (voltage * FULL_ANGLE) / GROVE_VCC;

    AzureIoT.publish("rotaryAngle", rotaryAngle);

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
