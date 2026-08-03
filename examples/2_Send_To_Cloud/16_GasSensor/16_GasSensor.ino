// Example 16 -- Gas Sensor (Grove MQ2)
//
// Grove Gas Sensor (MQ2) on analog pin A1, publishing the sensor's output
// voltage to Azure IoT Central. A higher voltage means more combustible
// gas / smoke detected. The MQ2 is analog and RELATIVE -- it isn't
// calibrated in ppm, so watch how the value changes rather than reading an
// absolute unit.
//
// NOTE: the MQ2 has a heater that needs to warm up (~20-30 s, longer on the
// very first use) before readings settle -- ignore the first few readings
// after power-on.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_GAS = 34;  // ESP32: GPIO 34 (ADC1). See the 12-bit voltage note below.
#else
const int PIN_GAS = A1;  // Uno WiFi Rev2 + Grove Base Shield: A1
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

    int sensorValue = analogRead(PIN_GAS);
    // Convert the raw ADC reading to a voltage. Uno: 10-bit ADC (0-1023) at
    // 5 V. On ESP32 the ADC is 12-bit (0-4095) at ~3.3 V -- change the
    // divisor to 4095.0f and the reference to 3.3f there for a correct volt.
    float gasVolt = sensorValue * (5.0f / 1023.0f);

    AzureIoT.publish("gas", gasVolt);

    delay(250); // loop() decides when to actually send
}
