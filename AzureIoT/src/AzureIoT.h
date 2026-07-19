#ifndef AZURE_IOT_H
#define AZURE_IOT_H
#include <Arduino.h>

// AzureIoT -- connects an Arduino Uno WiFi Rev2 to Azure IoT Central.
//
// Usage (see examples/ for complete sketches):
//
//   #include <AzureIoT.h>
//   #include "config.h"   // your Wi-Fi + Azure credentials, see config.h.example
//
//   void setup() {
//     Serial.begin(115200);
//     AzureIoT.begin();
//   }
//
//   void loop() {
//     AzureIoT.loop();                 // ALWAYS call this once per loop()
//
//     float tempC = /* your sensor reading + math, whatever you want */;
//     AzureIoT.publish("temperature", tempC);
//   }
//
// publish() just stages a value -- it does NOT send anything by itself.
// AzureIoT.loop() sends everything staged since the last send, as one
// combined MQTT/JSON message, on a fixed timer (see SEND_INTERVAL_MS in
// config.h). This keeps the message rate bounded and predictable no matter
// how many sensors you add or how often you call publish() -- important
// with ~20 boards sharing one classroom Wi-Fi network and one Azure app.
//
// Fields appear in the JSON in the order each key was FIRST published in
// your sketch's lifetime (not necessarily the order you call publish() in
// a given loop() iteration) -- harmless, since no JSON consumer (including
// IoT Central) cares about key order, but worth knowing if you're ever
// comparing two published messages by eye.
//
// If you specifically need to force an immediate send (e.g. reacting to a
// button press rather than waiting for the timer), call AzureIoT.sendNow().

class AzureIoTClass {
public:
    // Connects to Wi-Fi, provisions the device via DPS, and connects MQTT.
    // Blocks until successful (with visible retries) -- see the sketch's
    // Serial output while this runs.
    void begin();

    // Call this once per loop() iteration, every iteration. Handles Wi-Fi/
    // MQTT reconnection, SAS token refresh, and the automatic timed flush
    // of anything staged via publish().
    void loop();

    // Stages a key/value pair to be sent on the next automatic flush (or
    // the next sendNow() call). Calling this again with the same key
    // before the next send just overwrites the staged value.
    // IMPORTANT: pass a string literal for `key` (e.g. "temperature"), not
    // a value built at runtime (like String(...).c_str()) -- the pointer
    // is kept, not copied, so it must stay valid forever. String literals
    // always are; anything else might not be.
    void publish(const char *key, float value);

    // Forces an immediate send of everything currently staged, without
    // waiting for the automatic timer. Resets the timer afterwards.
    void sendNow();

private:
    bool ensureMqttConnected();
    void flush();
};

extern AzureIoTClass AzureIoT;

#endif
