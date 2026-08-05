// Example 10 -- LED Brightness (Numeric Writable Property)
//
// Demonstrates a NUMBER writable property: the dashboard sends a brightness
// level (0-255) that the board applies to a Grove LED via PWM, and the CLOUD
// REMEMBERS it. Unlike a command, a property is a persistent SETTING -- set
// the brightness while the board is OFF and it is applied the moment the board
// reconnects (via the one-time twin GET). This is onNumberProperty(), the
// numeric sibling of onBoolProperty().
//
// Command vs property, in one line: use a COMMAND for a one-shot action
// ("blink 3x"), use a NUMBER PROPERTY for a remembered setting ("brightness
// stays 128 until I change it, even across a reboot").
//
// IoT Central setup: Device templates > your template > click the model
// (the row tagged Root) >
//   + Add capability -> Capability type PROPERTY, Name "brightness",
//   Schema DOUBLE, tick Writable. Save ->
//   Views > Generate default views (a writable property -- number or bool --
//   only appears on the device page after a view is generated) -> Publish.
// On the device page, enter a value 0-255 and click Save.
//
// Setup: edit config.h in this folder with your Wi-Fi + Azure credentials
// before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_LED = 2;   // ESP32: onboard LED, PWM-capable via LEDC. GPIO 3 is Serial RX -- never drive it.
#else
const int PIN_LED = 3;   // Uno WiFi Rev2 + Grove Base Shield: Grove D3 (a PWM-capable pin for analogWrite)
#endif

// value arrives as a float; analogWrite() wants 0-255. Clamp defensively in
// case the dashboard sends something out of range.
void onBrightness(float value) {
    int level = (int)value;
    if (level < 0)   level = 0;
    if (level > 255) level = 255;
    analogWrite(PIN_LED, level);
    Serial.print("Brightness set from cloud: ");
    Serial.println(level);
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LED, OUTPUT);

    // Register BEFORE begin(). Persistent: on connect the board is told the
    // current cloud value automatically, so the LED comes up at the last
    // brightness you set -- even across a reboot.
    AzureIoT.onNumberProperty("brightness", onBrightness);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // handles reconnects AND delivering property updates to onBrightness()
}
