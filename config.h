#ifndef CONFIG_H
#define CONFIG_H

// ---- Wi-Fi ----
static const char WIFI_SSID[]     = "<your-ssid>";
static const char WIFI_PASSWORD[] = "<your-password>";

// ---- Azure IoT Central device identity ----
// From IoT Central: Devices > <device> > Connect
static const char IOTC_ID_SCOPE[]    = "<ID Scope, e.g. 0ne00XXXXXX>";
static const char IOTC_DEVICE_ID[]   = "<Device ID>";
static const char IOTC_DEVICE_KEY[]  = "<Primary or computed device key, base64>";

// DPS global endpoint (same for every IoT Central app worldwide)
static const char DPS_GLOBAL_HOST[] = "global.azure-devices-provisioning.net";

// How long a SAS token stays valid before we mint a new one (seconds).
// Kept short-ish so a stale token can't linger, but long enough to avoid
// constant re-signing (each sign = one SHA-256 HMAC over ~90 bytes, cheap).
static const unsigned long SAS_TOKEN_LIFETIME_SECS = 3600UL;

// Sensor read cadence
static const unsigned long SENSOR_READ_INTERVAL_MS = 5000UL;

#endif
