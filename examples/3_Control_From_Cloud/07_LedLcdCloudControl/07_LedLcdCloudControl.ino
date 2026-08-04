// Example 7 -- LED + LCD Cloud Control (Grove LCD RGB Backlight)
//
// The IoT Central dashboard's "ledState" toggle controls a Grove LED (pin 7)
// AND a Grove LCD RGB Backlight (I2C): the screen shows "LED: ON" / "LED:
// OFF" with a green/red backlight, so you can see the cloud command arrive
// even without staring at the LED. Same Mode-2 writable-property control as
// 01_LedCloudControl, with an LCD for display.
//
// Add a boolean writable property named "ledState" to your device template
// in IoT Central for the toggle to appear in the dashboard.
//
// NEEDS ONE EXTRA LIBRARY: the Seeed "Grove - LCD RGB Backlight" library,
// version 1.0.2 or newer (Library Manager). 1.0.2+ auto-detects both the
// v4.0 and v5.0 modules. The LCD is I2C -- plug it into any I2C port on the
// Grove Base Shield.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"
#include <Wire.h>
#include "rgb_lcd.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_LED = 2;   // ESP32: onboard LED. GPIO 7 is a flash pin on ESP32 -- never usable
#else
const int PIN_LED = 7;   // Uno WiFi Rev2 + Grove Base Shield: Grove D7
#endif

rgb_lcd lcd;

void onLedState(bool on) {
    digitalWrite(PIN_LED, on ? HIGH : LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cloud control:");
    lcd.setCursor(0, 1);
    lcd.print(on ? "LED: ON" : "LED: OFF");
    if (on) lcd.setRGB(0, 255, 0);   // green when on
    else    lcd.setRGB(255, 0, 0);   // red when off
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LED, OUTPUT);
    Wire.begin();
    lcd.begin(16, 2);
    lcd.print("Waiting for");     // shown until the first cloud value arrives
    lcd.setCursor(0, 1);
    lcd.print("cloud...");

    // Must be called BEFORE begin() -- see AzureIoT.h for the full design.
    AzureIoT.onBoolProperty("ledState", onLedState);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // delivers property updates to onLedState(), same call as always
}
