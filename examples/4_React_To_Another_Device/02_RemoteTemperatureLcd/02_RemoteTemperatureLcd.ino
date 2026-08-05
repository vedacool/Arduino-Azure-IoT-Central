// Example 2 -- Remote Temperature on LCD (Grove LCD RGB Backlight)
//
// Shows ANOTHER device's "temperature" telemetry live on a Grove LCD RGB
// Backlight (I2C). This device runs in PULL-ONLY MODE -- it connects Wi-Fi
// and polls Azure's REST API for the remote reading (no DPS/MQTT of its
// own), same as 01_RemoteTemperatureAlarm, but it DISPLAYS the value on the
// screen instead of sounding a buzzer.
//
// Needs two devices: Board A runs a temperature example (e.g. Send To Cloud
// 09_TemperatureSensor) publishing "temperature"; THIS board displays it.
//
// Setup:
// 1. Generate an IoT Central "API token" -- Permissions > API tokens > New >
//    role "App Operator" -- and put it in config.h as IOTC_REMOTE_API_TOKEN,
//    with IOTC_REMOTE_APP_SUBDOMAIN (your app's subdomain).
// 2. In config.h, set IOTC_REMOTE_DEVICE_ID to the Device ID (NOT the display
//    name) of the device publishing the "temperature".
//
// NEEDS ONE EXTRA LIBRARY: the Seeed "Grove - LCD RGB Backlight" library,
// version 1.0.2 or newer (Library Manager). 1.0.2+ auto-detects both the
// v4.0 and v5.0 modules. The LCD is I2C -- plug it into any I2C port on the
// Grove Base Shield.

#include <AzureIoT.h>
#include "config.h"
#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

void onRemoteTemperature(float value) {
    Serial.print("Remote temperature: ");
    Serial.println(value);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Remote temp:");
    lcd.setCursor(0, 1);
    lcd.print(value);
    lcd.print(" C");
}

void setup() {
    Serial.begin(9600);
    Wire.begin();
    lcd.begin(16, 2);
    lcd.print("Waiting for");
    lcd.setCursor(0, 1);
    lcd.print("remote data...");

    // Must be called BEFORE begin() -- see AzureIoT.h for the full design.
    AzureIoT.setRemoteAccess(IOTC_REMOTE_APP_SUBDOMAIN, IOTC_REMOTE_API_TOKEN);
    AzureIoT.onRemoteTelemetry("temperature", IOTC_REMOTE_DEVICE_ID, onRemoteTemperature);

    // Optional: how often to poll (default 15000ms, floor 1000ms). On Uno
    // WiFi Rev2, if you see repeated "poll FAILED ... HTTP status -1", raise
    // this (e.g. 60000) -- fewer TLS handshakes ease the WiFiNINA module.
    // AzureIoT.setRemotePollInterval(60000);

    // Pull-only mode (onRemoteTelemetry registered above): only WIFI_SSID/
    // WIFI_PASSWORD matter here. The identity args are ignored, so we pass
    // empty strings (the three IOTC_* identity values are commented out in
    // config.h).
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, "", "", "");
}

void loop() {
    AzureIoT.loop(); // polls the remote value and calls onRemoteTemperature()
}
