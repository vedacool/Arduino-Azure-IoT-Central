// Connection Test -- no sensor wiring needed at all. Sends a simple text
// message every few seconds to prove your Wi-Fi + Azure setup actually
// works, before you move on to any of the numbered sensor exercises.
//
// If this runs and you see the message updating in IoT Central, your
// Wi-Fi credentials, Azure IoT Central device credentials, and (on Uno
// WiFi Rev2) TLS certificate are all correct -- any trouble you hit in the
// sensor exercises after this is about the sensor/wiring, not the
// connection.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading. If you're on a Uno WiFi Rev2,
// see the AzureIoT library's main README for the one-time TLS certificate
// upload step (not needed on ESP32).

#include <AzureIoT.h>
#include "config.h"

const unsigned long SEND_EVERY_MS = 5000;
unsigned long lastSend = 0;
int counter = 0;

void setup() {
    Serial.begin(115200);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
    Serial.println("Connected! Sending a text message every 5 seconds...");
}

void loop() {
    AzureIoT.loop(); // ALWAYS call this every loop() -- handles reconnects

    if (millis() - lastSend >= SEND_EVERY_MS) {
        lastSend = millis();
        counter++;

        char message[48];
        snprintf(message, sizeof(message), "Hello from AzureIoT, message #%d", counter);

        // publishText() sends immediately (unlike publish(key, float), it
        // doesn't stage/batch) -- fine here since we're only sending one
        // simple message every 5 seconds ourselves, not high-rate telemetry.
        AzureIoT.publishText("message", message);
    }
}
