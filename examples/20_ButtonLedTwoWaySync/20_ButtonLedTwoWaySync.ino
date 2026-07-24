// Example 20 -- Button + LED Two-Way Sync
//
// The hardest example in this set -- combines a sensor (Grove Button,
// digital pin 4), an actuator (Grove LED, digital pin 7), and FULL
// two-way cloud control: the physical button toggles the LED locally
// AND tells the cloud about it, while the IoT Central dashboard toggle
// can ALSO turn the LED on/off directly. Whichever one acted most
// recently wins -- there's no "cloud always overrides local" or vice
// versa, which is what makes this the genuine two-way sync case the
// other examples in this set only do one direction of at a time
// (11/12 report-only, 15/16 cloud-control-only, 17/18/19 are
// automatic-plus-override, not a symmetric physical/cloud pair).
//
// Add a boolean writable property named "ledState" to your device
// template in IoT Central for the toggle to appear in the dashboard.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_BUTTON = 4;
#if defined(ARDUINO_ARCH_ESP32)
const int PIN_LED = 2;   // ESP32: onboard LED. GPIO 7 is a flash pin on ESP32 -- never usable
#else
const int PIN_LED = 7;   // Uno WiFi Rev2 + Grove Base Shield: Grove D7
#endif
const unsigned long DEBOUNCE_MS = 50; // ignore button-state flicker right after a change

bool ledState = false;
int lastButtonReading = LOW;
unsigned long lastButtonChangeMillis = 0;

void onLedState(bool on) {
    // Called when the CLOUD changes the property.
    ledState = on;
    digitalWrite(PIN_LED, ledState ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_LED, OUTPUT);

    AzureIoT.onBoolProperty("ledState", onLedState);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND delivering property updates to onLedState() above

    int reading = digitalRead(PIN_BUTTON);
    if (reading != lastButtonReading) {
        lastButtonChangeMillis = millis();
        lastButtonReading = reading;
    }

    // A press is a rising edge (LOW -> HIGH) that's held past the debounce
    // window -- toggle on the edge, not on every loop() while held.
    static bool armed = true; // true once the button has returned to released, ready for the next press
    if (millis() - lastButtonChangeMillis > DEBOUNCE_MS) {
        if (reading == HIGH && armed) {
            armed = false;
            ledState = !ledState;
            digitalWrite(PIN_LED, ledState ? HIGH : LOW);
            AzureIoT.reportBoolProperty("ledState", ledState); // tell the cloud the BUTTON changed it
        } else if (reading == LOW) {
            armed = true;
        }
    }

    AzureIoT.publish("button", (float)reading);
}
