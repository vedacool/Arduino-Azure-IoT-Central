// Module 4, Exercise 3 -- Sensor Bunyi (Sound Sensor)
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
    AzureIoT.begin();
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    // Same 32-sample averaging as the original Module 2/4 tutorials.
    long sum = 0;
    for (int i = 0; i < 32; i++) {
        sum += analogRead(PIN_SOUND);
    }
    sum >>= 5;

    AzureIoT.publish("sound", (float)sum);

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
