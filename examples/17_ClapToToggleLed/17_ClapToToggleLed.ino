// Example 17 -- Clap-to-Toggle LED
//
// Grove Sound Sensor on A2 detects a sharp sound spike (a clap) and toggles
// an LED on digital pin 7 -- an EVENT (a brief spike), not a sustained
// condition like light or temperature crossing a threshold. The LED is
// also cloud-controllable via onBoolProperty(), same as
// 15_LedCloudControl -- and because a clap is a local decision the cloud
// didn't make, reportBoolProperty() tells Azure about it, so the dashboard
// toggle reflects claps too, not just its own past commands.
//
// Add a boolean writable property named "ledState" to your device
// template in IoT Central for the toggle to appear in the dashboard.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_SOUND = A2;
#if defined(ARDUINO_ARCH_ESP32)
const int PIN_LED = 2;   // ESP32: onboard LED. GPIO 7 is a flash pin on ESP32 -- never usable
#else
const int PIN_LED = 7;   // Uno WiFi Rev2 + Grove Base Shield: Grove D7
#endif
const int CLAP_THRESHOLD = 100; // raw ADC deviation from quiet baseline -- adjust per environment
const unsigned long CLAP_COOLDOWN_MS = 500; // ignore further spikes right after one, so an echo doesn't double-toggle

bool ledState = false;
bool aboveThreshold = false; // tracks whether we're currently past the threshold, for edge detection
unsigned long lastToggleMillis = 0;

void onLedState(bool on) {
    // Called when the CLOUD changes the property -- keep our local
    // variable in sync so a later clap toggles from the right state.
    ledState = on;
    digitalWrite(PIN_LED, ledState ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_SOUND, INPUT);
    pinMode(PIN_LED, OUTPUT);

    AzureIoT.onBoolProperty("ledState", onLedState);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects, sending,
                      // AND delivering property updates to onLedState() above

    long sum = 0;
    for (int i = 0; i < 32; i++) {
        sum += analogRead(PIN_SOUND);
    }
    sum >>= 5;

    // Toggle on the RISING EDGE (quiet -> loud), gated by a cooldown --
    // edge detection alone stops SUSTAINED loud noise from retoggling every
    // loop() while it stays loud, but a real clap's decaying tail can still
    // flicker back and forth across the threshold for a short while as it
    // fades, which edge detection alone would count as several claps. The
    // cooldown blocks re-arming for a bit after a toggle, covering that
    // decay window without needing a fixed delay() in loop() (which would
    // also slow down AzureIoT.loop()'s reconnect/send handling).
    bool isAboveNow = sum > CLAP_THRESHOLD;
    if (isAboveNow && !aboveThreshold && millis() - lastToggleMillis > CLAP_COOLDOWN_MS) {
        lastToggleMillis = millis();
        ledState = !ledState;
        digitalWrite(PIN_LED, ledState ? HIGH : LOW);
        AzureIoT.reportBoolProperty("ledState", ledState); // tell the cloud WE changed it
    }
    aboveThreshold = isAboveNow;
}
