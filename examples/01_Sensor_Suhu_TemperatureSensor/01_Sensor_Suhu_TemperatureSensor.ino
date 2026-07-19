// Module 4, Exercise 1 -- Sensor Suhu (Temperature Sensor)
//
// Grove Temperature Sensor (thermistor) on analog pin A0, connected to
// Azure IoT Central via the AzureIoT library.
//
// Setup: copy config.h.example to config.h in this folder and fill in your
// Wi-Fi + Azure credentials before uploading. See the AzureIoT library's
// main README for the one-time TLS certificate upload step.

#include <AzureIoT.h>
#include "config.h"

const int PIN_TEMPERATURE = A0;

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    AzureIoT.begin();
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    // Same thermistor math as the original Module 2/4 tutorials.
    int a = analogRead(PIN_TEMPERATURE);
    if (a > 0) { // avoid divide-by-zero if the sensor isn't plugged in
        const int B = 4275;
        const long R0 = 100000;
        float R = 1023.0f / (float)a - 1.0f;
        R = (float)R0 * R;
        float temperature = 1.0f / (log(R / (float)R0) / B + 1.0f / 298.15f) - 273.15f;

        AzureIoT.publish("temperature", temperature);
    }

    delay(200); // read a few times a second; AzureIoT.loop() decides when to actually send
}
