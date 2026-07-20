// Example 14 -- Water Alarm
//
// Grove Water Sensor on digital pin 5 triggers BOTH a buzzer (digital pin
// 6) and an LED (digital pin 7) when water is detected -- the first
// example where one sensor reading drives more than one actuator at once.
// All three states are published as telemetry.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_WATER = 5;
const int PIN_BUZZER = 6;
const int PIN_LED = 7;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_WATER, INPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_LED, OUTPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int wet = digitalRead(PIN_WATER);

    digitalWrite(PIN_LED, wet ? HIGH : LOW);
    if (wet) {
        tone(PIN_BUZZER, 1000);
    } else {
        noTone(PIN_BUZZER);
    }

    AzureIoT.publish("water", (float)wet);
    AzureIoT.publish("ledState", wet ? 1.0f : 0.0f);
    AzureIoT.publish("buzzerOn", wet ? 1.0f : 0.0f);

    delay(500);
}
