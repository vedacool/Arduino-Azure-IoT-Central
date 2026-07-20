// Example 12 -- Buzzer Status Report
//
// Grove Buzzer on digital pin 6, beeping on a local timer. Same idea as
// 11_LedStatusReport: no cloud control, just reports its own on/off state
// as telemetry.
//
// tone()/noTone() are standard Arduino core calls, supported on both Uno
// WiFi Rev2 and modern ESP32 Arduino core versions (2.0.2+) -- if you're on
// an older ESP32 core without tone() support, update it via Boards Manager.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_BUZZER = 6;
const unsigned long BEEP_INTERVAL_MS = 1000;

bool buzzerOn = false;
unsigned long lastToggleMillis = 0;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUZZER, OUTPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    if (millis() - lastToggleMillis >= BEEP_INTERVAL_MS) {
        lastToggleMillis = millis();
        buzzerOn = !buzzerOn;
        if (buzzerOn) {
            tone(PIN_BUZZER, 1000);
        } else {
            noTone(PIN_BUZZER);
        }
        AzureIoT.publish("buzzerOn", buzzerOn ? 1.0f : 0.0f);
    }
}
