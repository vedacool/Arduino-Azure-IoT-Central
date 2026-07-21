// Example 19 -- Comfort Alarm (DHT11)
//
// Grove Temperature & Humidity Sensor (DHT11) on digital pin 2 triggers a
// buzzer (digital pin 6) automatically when it's uncomfortably hot/humid --
// same "automatic threshold + cloud override" pattern as
// 18_AutoNightLight, but with the trickier DHT11 sensor and a real-world
// use case: mute the alarm from the dashboard without unplugging anything.
//
// Two separate things are published here, deliberately not merged into
// one: "muted" is a writable PROPERTY (a control input -- the dashboard
// sets it), while "buzzerOn" is plain TELEMETRY (a read-only report of
// whether the buzzer is actually sounding right now). Giving these
// different names avoids a confusing situation where the same property
// means "please mute" when the cloud sets it and "currently buzzing" when
// the device reports it.
//
// This one needs one extra library beyond AzureIoT and its own dependencies:
// the Seeed "Grove Temperature And Humidity Sensor" library. Install it via
// Sketch > Include Library > Add .ZIP Library, using the .zip from
// https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor
//
// Add a boolean writable property named "muted" to your device template in
// IoT Central for the mute/unmute toggle to appear in the dashboard.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"
#include <DHT.h>

const int PIN_HUMIDITY = 2;
const int PIN_BUZZER = 6;
// Two thresholds each, not one -- a single threshold would flicker the
// buzzer on/off if the reading naturally hovers right at the boundary,
// which is a real risk here given DHT11's documented +/-2 degC / +/-5%
// accuracy. Sounding requires crossing the HIGH threshold; going quiet
// again requires dropping back below the LOW one.
const float TEMP_ALARM_HIGH_C = 30.0f;
const float TEMP_ALARM_LOW_C = 28.0f;          // adjust both to your idea of "uncomfortably hot"
const float HUMIDITY_ALARM_HIGH_PCT = 70.0f;
const float HUMIDITY_ALARM_LOW_PCT = 65.0f;    // adjust both to your idea of "uncomfortably humid"

DHT dht(PIN_HUMIDITY, DHT11);
bool buzzerOn = false;
bool muted = false; // set from the dashboard's "muted" toggle

void onMuted(bool on) {
    muted = on;
    if (muted && buzzerOn) {
        noTone(PIN_BUZZER);
        buzzerOn = false;
        AzureIoT.publish("buzzerOn", 0.0f); // report that muting silenced it, right away
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUZZER, OUTPUT);
    dht.begin();

    AzureIoT.onBoolProperty("muted", onMuted);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY, IOTC_MODEL_ID);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND delivering property updates to onMuted() above

    float temp_hum_val[2] = {0};
    if (dht.readTempAndHumidity(temp_hum_val) == 0) {
        float humidity = temp_hum_val[0];
        float temperature = temp_hum_val[1];
        AzureIoT.publish("humidity", humidity);
        AzureIoT.publish("temperature", temperature);

        bool shouldAlarm = buzzerOn; // stays the same unless a threshold is actually crossed
        if (!muted) {
            if (temperature > TEMP_ALARM_HIGH_C || humidity > HUMIDITY_ALARM_HIGH_PCT) {
                shouldAlarm = true;
            } else if (temperature < TEMP_ALARM_LOW_C && humidity < HUMIDITY_ALARM_LOW_PCT) {
                shouldAlarm = false;
            }
        } else {
            shouldAlarm = false;
        }
        if (shouldAlarm != buzzerOn) {
            buzzerOn = shouldAlarm;
            if (buzzerOn) {
                tone(PIN_BUZZER, 1000);
            } else {
                noTone(PIN_BUZZER);
            }
        }
        AzureIoT.publish("buzzerOn", buzzerOn ? 1.0f : 0.0f); // plain telemetry, not a writable property
    }

    delay(1000); // DHT11 only updates roughly once a second internally
}
