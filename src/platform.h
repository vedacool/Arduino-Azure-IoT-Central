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
    #include <esp_task_wdt.h>
    #include "azure_root_ca.h"
    typedef WiFiClientSecure SecureWiFiClient;
#else
    #include <WiFiNINA.h>
    #include <avr/io.h> // WDT.CTRLA / WDT_PERIOD_xxx_gc, for platformWatchdogEnable() below
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

// Enables a hardware watchdog that resets the device if platformWatchdogPet()
// isn't called for ~8 seconds -- catches an arbitrary hang (a wedged library
// call, a bug in the sketch's own loop()), not just "stuck disconnected"
// like connectWiFi()'s self-healing above. See AzureIoT.h's enableWatchdog()
// for the full design, including why this is opt-in and must be called
// AFTER begin(), not before.
inline void platformWatchdogEnable() {
#if defined(ARDUINO_ARCH_ESP32)
    // arduino-esp32 3.x enables its own default task watchdog already, with
    // a different shape than we want here -- must deinit before
    // reconfiguring. Also, 3.x changed esp_task_wdt_init()'s signature from
    // the old 2.x (timeoutSeconds, panic) pair to a config struct -- this
    // project targets 3.x (see library.properties), confirmed via the
    // arduino-esp32 project's own migration notes (the 2.x signature no
    // longer compiles at all on 3.x, so getting this wrong fails loudly at
    // compile time rather than silently misbehaving).
    esp_task_wdt_deinit();
    esp_task_wdt_config_t config = {
        .timeout_ms = 8000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = true
    };
    esp_task_wdt_init(&config);
    esp_task_wdt_add(NULL); // watch the current (loop) task
#else
    // Direct register write, deliberately NOT avr-libc's wdt_enable(WDTO_xxx)
    // -- that wrapper is documented (AVR Freaks forum; megaTinyCore/DxCore's
    // own reset-handling reference docs, from the maintainer of those
    // widely-used megaAVR-0 cores) as producing the WRONG period on
    // megaAVR-0 chips -- e.g. wdt_enable(WDTO_2S) observed resetting after
    // ~1s, not 2s, because avr-libc's WDTO_xxx timeout table was written for
    // classic AVR chips' different WDT hardware, not this chip's.
    // _PROTECTED_WRITE(WDT.CTRLA, ...) is the same CCP-protected-write
    // pattern reset.cpp already uses for RSTCTRL.SWRR, and matches
    // megaTinyCore/DxCore's own reference implementation exactly.
    // WDT_PERIOD_8KCLK_gc = ~8.2s, the maximum period this chip supports.
    _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_8KCLK_gc);
#endif
}

inline void platformWatchdogPet() {
#if defined(ARDUINO_ARCH_ESP32)
    // Only feed the TWDT if THIS task is actually subscribed. arduino-esp32
    // 3.x initializes a Task Watchdog at boot even when we never called
    // enableWatchdog() (i.e. esp_task_wdt_add()), so an UNCONDITIONAL reset
    // here logs "E task_wdt: esp_task_wdt_reset(): task not found" on every
    // pet during any http_client TLS read (DPS provisioning, remote-telemetry
    // polling) -- harmless (ESP-IDF just ignores the call) but alarming log
    // spam, seen on real ESP32 hardware. esp_task_wdt_status() returns ESP_OK
    // only when the running task is subscribed; otherwise (unsubscribed, or
    // TWDT not initialized) this becomes a genuine no-op -- which is exactly
    // what every platformWatchdogPet() call site already assumes (see the
    // "safe no-op if the watchdog was never enabled" comment in
    // http_client.cpp). On megaAVR that assumption was already true (a raw
    // WDR is harmless if the WDT is off); this makes it true on ESP32 too.
    if (esp_task_wdt_status(NULL) == ESP_OK) esp_task_wdt_reset();
#else
    // Raw WDR instruction, deliberately NOT avr-libc's wdt_reset() -- same
    // reasoning as platformWatchdogEnable() above: matches megaTinyCore/
    // DxCore's own reference implementation exactly, avoiding any avr-libc/
    // megaAVR-0 compatibility risk on the petting side too.
    __asm__ __volatile__ ("wdr" ::);
#endif
}

#endif
