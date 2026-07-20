#ifndef AZURE_IOT_H
#define AZURE_IOT_H
#include <Arduino.h>

// AzureIoT -- connects an Arduino Uno WiFi Rev2 or ESP32 board to Azure IoT
// Central.
//
// Usage (see examples/ for complete sketches):
//
//   #include <AzureIoT.h>
//   #include "config.h"   // your Wi-Fi + Azure credentials -- ships with placeholders, just edit them
//
//   void setup() {
//     Serial.begin(115200);
//     AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
//   }
//
//   void loop() {
//     AzureIoT.loop();                 // ALWAYS call this once per loop()
//
//     float tempC = /* your sensor reading + math, whatever you want */;
//     AzureIoT.publish("temperature", tempC);
//   }
//
// WHY begin() TAKES ARGUMENTS INSTEAD OF READING config.h ITSELF:
// config.h lives in your SKETCH folder, not inside this library. Arduino's
// build system does NOT extend a library's own .cpp files' #include search
// path to cover the sketch folder -- only the sketch's own .ino gets that.
// So a library file can never reliably do `#include "config.h"` and find a
// sketch-local file; an earlier version of this library tried exactly that
// and it broke depending on IDE/version. Passing the values in as plain
// arguments sidesteps the problem entirely: your sketch includes its own
// config.h (which it can always see, being in the same folder), and hands
// the values to the library as arguments.
//
// publish() just stages a value -- it does NOT send anything by itself.
// AzureIoT.loop() sends everything staged since the last send, as one
// combined MQTT/JSON message, on a fixed timer (see setSendInterval() below,
// default 5000ms). This keeps the message rate bounded and predictable no
// matter how many sensors you add or how often you call publish() --
// important with ~20 boards sharing one classroom Wi-Fi network and one
// Azure app.
//
// Fields appear in the JSON in the order each key was FIRST published in
// your sketch's lifetime (not necessarily the order you call publish() in
// a given loop() iteration) -- harmless, since no JSON consumer (including
// IoT Central) cares about key order, but worth knowing if you're ever
// comparing two published messages by eye.
//
// If you specifically need to force an immediate send (e.g. reacting to a
// button press rather than waiting for the timer), call AzureIoT.sendNow().
//
// LONG-RUN / UNATTENDED OPERATION:
// begin() and loop() both self-heal from network trouble in two tiers,
// deliberately kept separate because they have very different costs:
//
//   1. Wi-Fi down: after ~1 min (setWifiHardReinitAfter() to change), does a
//      full Wi-Fi radio reinit -- clears a wedged module/driver state a
//      plain retry can't. After ~5 min (setWifiForceResetAfter()) with
//      still no Wi-Fi, forces a full device reset. Neither of these ever
//      contacts Azure -- DPS isn't reached until Wi-Fi is actually up -- so
//      both thresholds can be as tight as you want with no downside.
//
//   2. DPS provisioning failing (Wi-Fi is fine, but Azure/config is the
//      problem): retries with exponential backoff (setProvisionRetryInitial()/
//      setProvisionRetryMax() to change, default 1min doubling to 15min)
//      instead of resetting. A reset can't fix a wrong ID scope or a
//      missing TLS certificate any faster than just trying again, and
//      resetting on a tight timer here would mean hammering Azure's DPS
//      endpoint repeatedly during a real outage, which risks the device
//      being rate-limited. Worst case (permanent misconfiguration), this
//      settles into a slow, harmless retry loop forever rather than
//      bricking the device or spamming Azure.
//
// The reset in tier 1 uses RSTCTRL.SWRR on megaAVR boards (Uno WiFi Rev2's
// dedicated software-reset register) and ESP.restart() on ESP32 -- see
// reset.h/reset.cpp for details and verification caveats.

class AzureIoTClass {
public:
    // Connects to Wi-Fi, provisions the device via DPS, and connects MQTT.
    // Blocks until successful (with visible retries) -- see the sketch's
    // Serial output while this runs. Call any of the setters below BEFORE
    // this, if you want to change their defaults.
    void begin(const char *wifiSsid, const char *wifiPassword,
               const char *idScope, const char *deviceId, const char *deviceKey);

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

    // Sends a single string key/value pair as its own message, IMMEDIATELY
    // -- unlike publish(key, float), this does not stage/batch/wait for the
    // timer. Meant for occasional status messages and connection testing
    // (e.g. "AzureIoT.publishText("status", "online");" from a sketch with
    // no sensors wired up yet, just to confirm Wi-Fi/DPS/MQTT all work) --
    // not for frequent telemetry, since it bypasses the send-rate bounding
    // that publish()/loop() give you for free. Handles JSON-escaping the
    // text for you.
    void publishText(const char *key, const char *value);

    // ---- Optional tuning, call before begin() to override a default ----

    // DPS endpoint (default: the global Azure IoT Central endpoint). Only
    // needed for Azure Government or Azure China, which use different
    // endpoints.
    void setDpsGlobalHost(const char *host);

    // How long a SAS token stays valid before a new one is minted (default
    // 3600s / 1 hour).
    void setSasTokenLifetime(unsigned long seconds);

    // How often loop() sends whatever's been staged via publish() (default
    // 5000ms). Call publish() as often as you like -- this is what actually
    // controls the send rate.
    void setSendInterval(unsigned long ms);

    // How long to wait for one Wi-Fi connect attempt before retrying
    // (default 20000ms).
    void setWifiConnectTimeout(unsigned long ms);

    // Minimum time between MQTT reconnect attempts when disconnected
    // (default 5000ms) -- stops loop() from hammering Azure with reconnects
    // if the network is down.
    void setMqttReconnectCooldown(unsigned long ms);

    // Wi-Fi self-healing thresholds -- see the long-run notes above
    // (defaults: 60000ms / 1 min, and 300000ms / 5 min).
    void setWifiHardReinitAfter(unsigned long ms);
    void setWifiForceResetAfter(unsigned long ms);

    // DPS provisioning retry backoff -- see the long-run notes above
    // (defaults: 60000ms / 1 min initial, 900000ms / 15 min cap).
    void setProvisionRetryInitial(unsigned long ms);
    void setProvisionRetryMax(unsigned long ms);

private:
    bool ensureMqttConnected();
    void flush();
};

extern AzureIoTClass AzureIoT;

#endif
