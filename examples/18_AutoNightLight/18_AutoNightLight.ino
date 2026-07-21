// Example 18 -- Auto Night Light
//
// Grove Light Sensor on A4 automatically turns an LED (digital pin 7) on
// when it's dark and off when it's bright -- a SUSTAINED condition
// (unlike 17_ClapToToggleLed's momentary event). The LED is also
// cloud-controllable via onBoolProperty(): flip the dashboard toggle to
// manually override the automatic behavior (e.g. force it off during the
// day, or on even when it's not dark). The next automatic light-level
// crossing will take back over -- this example doesn't try to "remember"
// that you overrode it, keeping the logic simple.
//
// Add a boolean writable property named "ledState" to your device
// template in IoT Central for the toggle to appear in the dashboard.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_LIGHT = A4;
const int PIN_LED = 7;
// Two thresholds, not one -- a single threshold would flicker the LED
// rapidly if the light level naturally hovers right at the boundary (ADC
// noise, a cloud passing overhead at dusk). Turning on requires dropping
// below the LOW threshold; turning back off requires rising above the
// HIGH one -- the gap between them (the "dead band") is what stops flicker.
const int DARK_THRESHOLD_LOW = 280;  // turns ON once below this
const int DARK_THRESHOLD_HIGH = 320; // turns OFF once above this -- adjust both per environment

bool ledState = false;

void onLedState(bool on) {
    ledState = on;
    digitalWrite(PIN_LED, ledState ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LIGHT, INPUT);
    pinMode(PIN_LED, OUTPUT);

    AzureIoT.onBoolProperty("ledState", onLedState);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND delivering property updates to onLedState() above

    int lightLevel = analogRead(PIN_LIGHT);
    bool shouldBeOn = ledState; // stays the same unless a threshold is actually crossed
    if (lightLevel < DARK_THRESHOLD_LOW) shouldBeOn = true;
    else if (lightLevel > DARK_THRESHOLD_HIGH) shouldBeOn = false;

    if (shouldBeOn != ledState) {
        ledState = shouldBeOn;
        digitalWrite(PIN_LED, ledState ? HIGH : LOW);
        AzureIoT.reportBoolProperty("ledState", ledState); // tell the cloud the light level decided this
    }

    AzureIoT.publish("light", (float)lightLevel);

    delay(200);
}
