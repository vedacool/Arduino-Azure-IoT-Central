# Arduino Uno WiFi Rev2 → Azure IoT Central

Connects your board straight to Azure IoT Central — no PC-side tools, no editing library files. Replaces the older `firedog1024/arduino-uno-wifi-iotc` + `Azure/dps-keygen` approach (the latter is archived and its connection-string feature stopped working in 2020).

---

## Start here

Follow these in order. Each one only needs to be done once.

### 1. Get your Azure IoT Central device credentials

In your IoT Central app: **Devices → your device → Connect**. You'll need three values:
- **ID scope**
- **Device ID**
- **Primary key** (or a computed device key, if you're using a group enrollment)

### 2. Upload Azure's TLS certificate to your board

This step can't be done from the sketch — it has to happen once via the Arduino IDE, and it's the single most common reason a fresh Azure/Arduino project fails to connect.

1. Arduino IDE → **Tools → WiFiNINA Firmware/Certificates Updater**
2. Add domain → type `global.azure-devices-provisioning.net` → Upload
3. (You'll add one more domain after Step 5 below — the sketch tells you which one.)

### 3. Install the required libraries

Arduino IDE → **Sketch → Include Library → Manage Libraries**, install:
- `WiFiNINA`
- `PubSubClient` (must be v2.8 or newer — check the version shown in Library Manager)

### 4. Set up your config file

In this folder, copy `config.h.example` to a new file named `config.h`, then edit it:
- `WIFI_SSID` / `WIFI_PASSWORD` — your Wi-Fi credentials
- `IOTC_ID_SCOPE` / `IOTC_DEVICE_ID` / `IOTC_DEVICE_KEY` — from Step 1
- Leave everything else at its default for now

**`config.h` is deliberately left out of git** (check `.gitignore`) so your real Wi-Fi password and Azure key never end up committed anywhere. Only `config.h.example`, with placeholder values, is tracked.

### 5. Upload and check the Serial Monitor

Open `uno_wifi_azure.ino`, select **Tools → Board → Arduino Uno WiFi Rev2** and the right port, then Upload. Open **Tools → Serial Monitor** at **115200 baud**. You should see:

```
Connecting to WiFi: ...
WiFi connected, IP: ...
Waiting for network time...
Provisioning via DPS...
Assigned hub: iotc-xxxxxxxx-....azure-devices.net
Connecting to MQTT as ...
MQTT connected.
Published: {"temperature":23.45}
```

Copy that **"Assigned hub"** hostname, go back to the Certificates Updater from Step 2, add that domain too, and re-upload the sketch. From this point on it'll keep working even if IoT Central ever reassigns you to a different hub — the sketch re-provisions on every boot.

### 6. Check the data is arriving in IoT Central

IoT Central → **Devices → your device → View** — you should see the `temperature` field updating every 5 seconds.

**If something's not working**, see [Troubleshooting](#troubleshooting) below before assuming the code is broken.

---

## Adding your own sensors

This is the part built for the workshop's "sensors" projects — adding a sensor takes two small edits in `sensors.cpp`, and nothing else in the project needs to change.

**Turning on a sensor that's already built in** (temperature, rotary angle, sound, touch, turbidity — matching the Grove modules from Modules 2 & 4): open `config.h` and flip its `ENABLE_SENSOR_*` flag to `1`, e.g.:

```c
#define ENABLE_SENSOR_ROTARY_ANGLE 1
#define PIN_ROTARY_ANGLE           A1
```

That's it — re-upload, and `rotaryAngle` will start appearing in the published JSON alongside whatever else is enabled. Disabled sensors cost zero flash/RAM; they're compiled out entirely.

**Adding a brand-new sensor** — two edits in `sensors.cpp`:

1. Write a read function, following the same pattern as the ones already there:
   ```c
   static bool read_mySensor(float *out) {
       int raw = analogRead(PIN_MY_SENSOR);
       *out = /* your conversion math */;
       return true; // return false instead if the reading looks invalid
   }
   ```
2. Add one line to `g_sensorTable[]` at the bottom of the same file:
   ```c
   { "mySensorKey", read_mySensor },
   ```

That's the entire integration. You don't touch `uno_wifi_azure.ino`, the MQTT code, or the SAS-token code — the payload builder reads `g_sensorTable` generically and publishes whatever's in it as `{"mySensorKey": value, ...}`.

Full sensor list, pins, and enable flags live in `config.h.example`. The DHT11 humidity/temperature sensor needs one extra library (noted in a comment right above `read_humidity()` in `sensors.cpp`) since Azure/MQTT itself doesn't need it — only that specific sensor does.

---

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Hangs at "Connecting to WiFi" with dots forever, then retries | Wrong `WIFI_SSID`/`WIFI_PASSWORD` in `config.h`, or board out of range |
| "DPS provisioning FAILED" | Wrong `IOTC_ID_SCOPE`/`IOTC_DEVICE_ID`/`IOTC_DEVICE_KEY`, **or** you haven't done Step 2 (TLS certificate) yet |
| Provisioning succeeds but "MQTT connect failed, rc=..." | You haven't added the *assigned hub's* domain to the Certificates Updater yet (see end of Step 5) |
| Nothing appears in IoT Central even though Serial shows "Published: ..." | Check the device is using the same device template/capability model your dashboard expects — a raw JSON key like `temperature` needs a matching capability in IoT Central for the dashboard to render it |
| Board resets or hangs after running for a while | See "Known limitations" below — this hasn't been endurance-tested on real hardware yet |

---

## Files

| File | What it's for |
|---|---|
| `uno_wifi_azure.ino` | Setup/loop: WiFi connect, provisioning, MQTT connect, publish cycle |
| `config.h.example` | Template — copy to `config.h` and fill in your credentials |
| `sensors.h` / `sensors.cpp` | **Edit this to add sensors.** Table-driven: one function + one table row per sensor |
| `dps_client.h` / `dps_client.cpp` | On-device DPS registration (replaces the PC-side `dps-keygen` tool) |
| `sas_token.h` / `sas_token.c` | Builds the Azure SAS token string |
| `sha256.h`/`.c`, `b64url.h`/`.c` | SHA-256, HMAC-SHA256, base64, URL-encoding — the crypto primitives SAS tokens need |

---

## Known limitations (read before a long unattended run)

- **Peak stack depth** during DPS provisioning is roughly 1.2–1.5KB on a board with 6KB total SRAM. It's been verified to compile and run correctly in a simulated test, but not endurance-tested on real hardware yet — worth keeping an eye on if you add several more sensors or a much longer device key.
- **No watchdog timer.** If the board ever hangs (e.g. an unusual network edge case), it stays hung until manually power-cycled.
- **`extractJsonString()`** (in `dps_client.cpp`) is a lightweight substring search, not a full JSON parser. It matches Azure's documented DPS response format but isn't robust to Azure changing that format's structure.
- The board's **ATECC608 crypto chip goes unused** — authentication here is a software-computed SAS token (symmetric key), not the hardware-backed X.509 approach the chip is designed for. That'd be a bigger redesign.

## What's been verified (and how)

- **Crypto core** (`sha256.c`/`b64url.c`/`sas_token.c`): checked against NIST/RFC test vectors and, separately, byte-for-byte against an independent Python (`hashlib`/`hmac`/`base64`) implementation of Azure's documented SAS-token algorithm across 16 randomized cases.
- **API usage**: every WiFiNINA/PubSubClient call checked against the actual current library source.
- **Compiles, links, and runs end-to-end**: the full sketch source (all sensors, all code paths) was compiled and linked against signature-accurate mocks of the real libraries and run — this process caught and fixed a real buffer-overflow bug in `base64_decode`, a missing `#include`, and missing `extern "C"` linkage guards, before any of it touched real hardware.
- **What's not verified**: real hardware, real Azure credentials, and the exact `atmega4809` compiler device pack (only available through Arduino's own Board Manager download, unreachable from the sandbox this was built in). Compile it once for real and test against one real device before relying on it for a group of students.
