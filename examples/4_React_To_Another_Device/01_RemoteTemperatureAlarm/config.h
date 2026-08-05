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

// ---- Azure IoT Central device identity (THIS device's own connection) ----
// NOT USED by this example, so these are commented out on purpose. Because
// onRemoteTelemetry() is registered in the .ino before begin(), this device
// runs in PULL-ONLY MODE (Wi-Fi only, no DPS/MQTT -- see the .ino header and
// AzureIoT.h's begin() doc comment for why), so it never provisions an
// identity of its own. The .ino passes empty strings ("") for these three.
// You do NOT need to fill anything in here -- leave them commented out.
// static const char IOTC_ID_SCOPE[]    = "<ID Scope, e.g. 0ne00XXXXXX>";
// static const char IOTC_DEVICE_ID[]   = "<Device ID>";
// static const char IOTC_DEVICE_KEY[]  = "<Primary or computed device key, base64>";

// ---- Remote telemetry access (a DIFFERENT credential -- see AzureIoT.h's
// onRemoteTelemetry() for why this is separate from the device identity
// above) ----
// From IoT Central: Permissions > API tokens > New > role "App Operator".
// IOTC_REMOTE_APP_SUBDOMAIN is just the subdomain part -- e.g. if your app
// is at "myapp.azureiotcentral.com", this is "myapp", not the full URL.
static const char IOTC_REMOTE_APP_SUBDOMAIN[] = "<your-app-subdomain>";
static const char IOTC_REMOTE_API_TOKEN[]     = "<SharedAccessSignature sr=...&sig=...&skn=...&se=...>";

// The OTHER board whose "temperature" this board watches. Use its Device
// *ID*, NOT its display name -- in IoT Central a device has a friendly
// Device *name* (e.g. "arduino wifi uno rev2") AND a separate Device *ID*
// (e.g. "arduino-wifi"); the REST API only accepts the ID. Copy it from
// Devices > [that device], the "Device ID" field (lowercase, no spaces).
static const char IOTC_REMOTE_DEVICE_ID[] = "<the OTHER board's Device ID, e.g. arduino1>";

// That's it -- the first five are passed straight into AzureIoT.begin() in
// this sketch's setup(). Everything else (send interval, timeouts, the
// Wi-Fi self-healing thresholds, DPS retry backoff, the DPS endpoint for
// Azure Government/China) now has a sensible default built into the
// library itself, and can be overridden with a setter if you ever need to
// -- see AzureIoT.h for the full list (setSendInterval(), setDpsGlobalHost(),
// etc.), called before AzureIoT.begin() in setup().

#endif
