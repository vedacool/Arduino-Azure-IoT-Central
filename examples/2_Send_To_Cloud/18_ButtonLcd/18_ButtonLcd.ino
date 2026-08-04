// Example 18 -- Button + LCD (Grove LCD RGB Backlight)
//
// The simplest way to see the LCD working: a Grove Button on digital pin 4,
// and a Grove LCD RGB Backlight (I2C) that shows "Pressed" or "Released" as
// you press it. The button state is also published to Azure IoT Central, so
// this is still a normal Mode-1 (device -> cloud) example -- the LCD is just
// local feedback on top.
//
// NEEDS ONE EXTRA LIBRARY: the Seeed "Grove - LCD RGB Backlight" library,
// version 1.0.2 or newer (Library Manager -> search "Grove LCD RGB
// Backlight"). 1.0.2+ auto-detects BOTH the v4.0 and v5.0 hardware modules
// (they use different backlight chips) -- an older version only drives the
// v4.0 backlight. The LCD is I2C: plug it into any I2C port on the Grove
// Base Shield -- there's no pin number to set for it.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"
#include <Wire.h>
#include "rgb_lcd.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_BUTTON = 27;  // ESP32: a safe digital-input GPIO
#else
const int PIN_BUTTON = 4;   // Uno WiFi Rev2 + Grove Base Shield: Grove D4
#endif

rgb_lcd lcd;
int lastState = -1; // -1 forces the first draw

void showState(int pressed) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Button:");
    lcd.setCursor(0, 1);
    lcd.print(pressed ? "Pressed" : "Released");
    if (pressed) lcd.setRGB(0, 255, 0);    // green while pressed
    else         lcd.setRGB(60, 60, 60);   // dim white while released
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_BUTTON, INPUT);
    Wire.begin();
    lcd.begin(16, 2);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int pressed = digitalRead(PIN_BUTTON);
    if (pressed != lastState) { // only redraw the LCD when the state actually changes (no flicker)
        lastState = pressed;
        showState(pressed);
    }
    AzureIoT.publish("button", (float)pressed);

    delay(100); // a button is a fast-changing input; loop() decides when to actually send
}
