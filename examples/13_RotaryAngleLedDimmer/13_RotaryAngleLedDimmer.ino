// Example 13 -- Rotary Angle -> LED Dimmer
//
// Grove Rotary Angle Sensor on A1 controls an LED's brightness on PWM pin
// 3 via analogWrite() -- the first example in this set to use analog
// OUTPUT, not just analog input. The current angle (in degrees) is also
// published as telemetry.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

#define ROTARY_ANGLE_SENSOR A1
#define LED_PIN 3 // must be a PWM-capable pin for analogWrite() to work
#define ADC_REF 5
#define FULL_ANGLE 300
#define VCC_GROVE 5

void setup() {
    Serial.begin(115200);
    pinMode(ROTARY_ANGLE_SENSOR, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int sensorValue = analogRead(ROTARY_ANGLE_SENSOR);
    float voltage = (float)sensorValue * ADC_REF / 1023.0f;
    float degree = (voltage * FULL_ANGLE) / VCC_GROVE;

    int brightness = (int)map((long)degree, 0, FULL_ANGLE, 0, 255);
    analogWrite(LED_PIN, brightness);

    AzureIoT.publish("angle", degree);

    delay(200);
}
