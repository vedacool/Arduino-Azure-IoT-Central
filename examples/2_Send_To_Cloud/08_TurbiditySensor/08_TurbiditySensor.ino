// Example 8 -- Turbidity Sensor
//
// Grove Turbidity Sensor on analog pin A3, connected to Azure IoT Central
// via the AzureIoT library.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_TURBIDITY = 39;  // ESP32: GPIO 39 (ADC1). Re-tune the conversion for the 12-bit ADC.
#else
const int PIN_TURBIDITY = A3;  // Uno WiFi Rev2 + Grove Base Shield: A3
#endif

void setup() {
    Serial.begin(9600);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    // Standard Grove turbidity sensor conversion math.
    int sensorValue = analogRead(PIN_TURBIDITY);
    float turbidity = sensorValue * (5.0f / 1024.0f);

    AzureIoT.publish("turbidity", turbidity);

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
