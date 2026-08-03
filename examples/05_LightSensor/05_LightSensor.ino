// Example 5 -- Light Sensor
//
// Grove Light Sensor (photoresistor) on analog pin A4, connected to
// Azure IoT Central via the AzureIoT library. Publishes the raw ADC
// reading (0-1023, higher = brighter).
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_LIGHT = 35;  // ESP32: GPIO 35 (ADC1). Re-tune thresholds for the 12-bit ADC.
#else
const int PIN_LIGHT = A4;  // Uno WiFi Rev2 + Grove Base Shield: A4
#endif

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LIGHT, INPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int sensorValue = analogRead(PIN_LIGHT);
    AzureIoT.publish("light", (float)sensorValue);

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
