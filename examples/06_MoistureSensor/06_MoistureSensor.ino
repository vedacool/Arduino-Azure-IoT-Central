// Example 6 -- Moisture Sensor
//
// Grove Moisture Sensor on analog pin A5, connected to Azure IoT Central
// via the AzureIoT library. Publishes the raw ADC reading (0-1023, higher
// = wetter). Don't insert the sensor into soil past its "highest position"
// line.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_MOISTURE = A5;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_MOISTURE, INPUT);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    int sensorValue = analogRead(PIN_MOISTURE);
    AzureIoT.publish("moisture", (float)sensorValue);

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
