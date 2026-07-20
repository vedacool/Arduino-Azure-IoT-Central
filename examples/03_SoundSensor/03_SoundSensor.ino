// Exercise 3 -- Sound Sensor
//
// Grove Sound Sensor on analog pin A2, connected to Azure IoT Central via
// the AzureIoT library.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

const int PIN_SOUND = A2;

void setup() {
    Serial.begin(115200);

    // Optional: how often AzureIoT.loop() sends staged publish() data
    // (default 5000ms / 5 seconds). Uncomment and adjust if you want it
    // faster or slower -- must be called before begin().
    // AzureIoT.setSendInterval(5000);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    // 32-sample averaging to smooth out sensor noise.
    long sum = 0;
    for (int i = 0; i < 32; i++) {
        sum += analogRead(PIN_SOUND);
    }
    sum >>= 5;

    AzureIoT.publish("sound", (float)sum);

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
