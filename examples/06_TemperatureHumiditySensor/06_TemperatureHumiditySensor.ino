// Exercise 6 -- Temperature & Humidity Sensor
//
// Grove Temperature & Humidity Sensor (DHT11) on digital pin 2, connected
// to Azure IoT Central via the AzureIoT library.
//
// This one needs one extra library beyond AzureIoT and its own dependencies:
// the Seeed "Grove Temperature And Humidity Sensor" library. Install it via
// Sketch > Include Library > Add .ZIP Library, using the .zip from
// https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"
#include <DHT.h>

const int PIN_HUMIDITY = 2;
DHT dht(PIN_HUMIDITY, DHT11);

void setup() {
    Serial.begin(115200);
    dht.begin();

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    float temp_hum_val[2] = {0};
    if (dht.readTempAndHumidity(temp_hum_val) == 0) {
        float humidity = temp_hum_val[0];
        float temperature = temp_hum_val[1];
        AzureIoT.publish("humidity", humidity);
        AzureIoT.publish("temperature", temperature);
    }

    delay(1000); // DHT11 only updates roughly once a second internally
}
