#ifndef PLATFORM_H
#define PLATFORM_H

// Isolates every WiFiNINA-vs-ESP32 difference to this one file, so the rest
// of the library (dps_client.cpp, AzureIoT.cpp) doesn't need to care which
// board it's running on.

#if defined(ARDUINO_ARCH_ESP32)
    #include <WiFi.h>
    #include <WiFiClientSecure.h>
    #include <time.h>
    #include <Esp.h>
    #include "azure_root_ca.h"
    typedef WiFiClientSecure SecureWiFiClient;
#else
    #include <WiFiNINA.h>
    typedef WiFiSSLClient SecureWiFiClient;
#endif

// Configures TLS trust for a freshly-constructed secure client, before its
// first connect(). On WiFiNINA boards (Uno WiFi Rev2), trust is configured
// once via the Arduino IDE's Firmware/Certificates Updater tool, so this is
// a no-op. ESP32 has no such tool -- WiFiClientSecure needs the CA cert
// handed to it directly, so this supplies the one embedded in
// azure_root_ca.cpp.
inline void configureSecureClient(SecureWiFiClient &client) {
#if defined(ARDUINO_ARCH_ESP32)
    client.setCACert(AZURE_ROOT_CA);
#else
    (void)client;
#endif
}

// Returns current UNIX time. WiFiNINA modules keep an NTP-synced clock
// automatically once connected. ESP32 has no equivalent built into WiFi.h --
// needs an explicit one-time NTP sync via platformBeginTimeSync() first.
inline unsigned long platformGetUnixTime() {
#if defined(ARDUINO_ARCH_ESP32)
    return (unsigned long)time(nullptr);
#else
    return WiFi.getTime();
#endif
}

// Call once, after Wi-Fi connects, before relying on platformGetUnixTime().
// No-op on WiFiNINA boards (they sync automatically); starts an NTP sync on
// ESP32 (UTC, no DST offset -- SAS tokens only care about the UNIX epoch).
inline void platformBeginTimeSync() {
#if defined(ARDUINO_ARCH_ESP32)
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
#endif
}

// The "still down after ~1 min" escalation tier: reinitialize the Wi-Fi
// radio rather than just retrying WiFi.begin() again.
inline void platformWifiHardReinit() {
#if defined(ARDUINO_ARCH_ESP32)
    WiFi.disconnect(true); // true = also turn the radio off, not just drop the AP
#else
    WiFi.end();
#endif
}

#endif
