// Example 9 -- Temperature Sensor
//
// Grove Temperature Sensor (thermistor) on analog pin A0, connected to
// Azure IoT Central via the AzureIoT library.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading. If you're on a Uno WiFi Rev2,
// see the AzureIoT library's main README for the one-time TLS certificate
// upload step (not needed on ESP32).

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_TEMPERATURE = 36;  // ESP32: GPIO 36 (ADC1). Re-tune the thermistor math for the 12-bit ADC.
#else
const int PIN_TEMPERATURE = A0;  // Uno WiFi Rev2 + Grove Base Shield: A0
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

    // Standard Grove thermistor conversion math.
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
