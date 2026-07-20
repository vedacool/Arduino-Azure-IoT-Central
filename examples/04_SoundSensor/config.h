#ifndef CONFIG_H
#define CONFIG_H

// This file is tracked in git WITH the placeholder values below, on purpose
// -- so this example compiles the moment you download it, no copy-a-file
// step needed (same pattern Arduino's own examples use for
// arduino_secrets.h). Just edit the values in place.
//
// If you're developing a fork of THIS repo itself (not just using it as a
// library), remember this file is no longer gitignored: don't commit your
// real Wi-Fi password or Azure key here. For your own separate project
// that happens to use this library, this doesn't apply -- there's no git
// repo tracking your Arduino sketch folder unless you make one yourself.

// ---- Wi-Fi ----
static const char WIFI_SSID[]     = "<your-ssid>";
static const char WIFI_PASSWORD[] = "<your-password>";

// ---- Azure IoT Central device identity ----
// From IoT Central: Devices > <device> > Connect
static const char IOTC_ID_SCOPE[]    = "<ID Scope, e.g. 0ne00XXXXXX>";
static const char IOTC_DEVICE_ID[]   = "<Device ID>";
static const char IOTC_DEVICE_KEY[]  = "<Primary or computed device key, base64>";

// That's it -- these five are passed straight into AzureIoT.begin() in
// this sketch's setup(). Everything else (send interval, timeouts, the
// Wi-Fi self-healing thresholds, DPS retry backoff, the DPS endpoint for
// Azure Government/China) now has a sensible default built into the
// library itself, and can be overridden with a setter if you ever need to
// -- see AzureIoT.h for the full list (setSendInterval(), setDpsGlobalHost(),
// etc.), called before AzureIoT.begin() in setup().

#endif
