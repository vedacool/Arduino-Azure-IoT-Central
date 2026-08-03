// Example 24 -- PIR Motion Sensor (Grove)
//
// Grove PIR Motion Sensor on digital pin 2, publishing 1 when motion is
// detected and 0 otherwise to Azure IoT Central. Same simple digital-in
// shape as the Touch and Button examples.
//
// (To also sound a buzzer or light an LED on motion, combine this with an
// actuator the way 14_WaterAlarm does -- read here, act there.)
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_PIR = 14;  // ESP32: a safe digital-input GPIO
#else
const int PIN_PIR = 2;   // Uno WiFi Rev2 + Grove Base Shield: Grove D2
#endif

void setup() {
    Serial.begin(115200);
    pinMode(PIN_PIR, INPUT);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int motion = digitalRead(PIN_PIR); // HIGH (1) when the sensor sees movement
    AzureIoT.publish("motion", (float)motion);

    delay(200); // motion is a fast-changing input; loop() decides when to actually send
}
