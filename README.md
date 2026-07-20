# AzureIoT — Arduino Uno WiFi Rev2 or ESP32 → Azure IoT Central

An Arduino library that connects a Uno WiFi Rev2 **or an ESP32 board** to Azure IoT Central: Wi-Fi, on-device Azure DPS provisioning, and MQTT telemetry — no PC-side tools, no editing library files, no config-table to learn.

If you just want to get a Uno WiFi Rev2 or ESP32 board talking to Azure IoT Central with a couple of function calls, skip straight to **Start here** below. The `examples/` folder has six ready-to-run sketches, one per Grove sensor type, each a short, complete sketch you can read top to bottom — plus a connection-test sketch that needs no sensor at all.

---

## Start here

### 1. Install the library

**Easiest: Arduino IDE → Sketch → Include Library → Add .ZIP Library...** — download this repo as a ZIP (green **Code** button → **Download ZIP** on GitHub) and select that file directly. No unzipping needed.

**Alternative (manual):** clone or unzip this repo, then rename the resulting folder to `AzureIoT` and place it directly in your Arduino libraries folder:
- Windows/Mac/Linux: `Documents/Arduino/libraries/AzureIoT`

Either way, restart the Arduino IDE afterward. You should now see **File → Examples → AzureIoT** listing the six examples.

### 2. Board-specific setup

**If you're using an ESP32 board:**
- Install ESP32 board support if you haven't already: **File → Preferences** → add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to **Additional Board Manager URLs**, then **Tools → Board → Boards Manager** → search "esp32" → install the one **by Espressif Systems** (there can be similarly-named third-party entries — Espressif Systems is the correct, official one).
- Install the one dependency: **Sketch → Include Library → Manage Libraries** → `PubSubClient` (must be v2.8 or newer).
- WiFi and TLS come from the ESP32 core itself (`WiFi.h`/`WiFiClientSecure.h`) — nothing else to install. The Azure root certificate is embedded directly in this library (`src/azure_root_ca.cpp`), so **there's no certificate-upload step at all** on ESP32 — skip Step 4 below entirely.

**If you're using an Arduino Uno WiFi Rev2:**
- Install board support if you haven't already: **Tools → Board → Boards Manager** → search "megaAVR" → install **Arduino megaAVR Boards, by Arduino** (this is what makes "Arduino Uno WiFi Rev2" show up under Tools → Board). No Additional Board Manager URL needed — it's an official Arduino package.
- Install two dependencies: **Sketch → Include Library → Manage Libraries** → `WiFiNINA` and `PubSubClient` (v2.8+).
- The TLS certificate step described in Step 4 below *might* not even be necessary — try `00_ConnectionTest` first (see Step 5) before doing anything with certificates.

### 3. Get your Azure IoT Central device credentials

In your IoT Central app: **Devices → your device → Connect**. You'll need:
- **ID scope**
- **Device ID**
- **Primary key** (or a computed device key, if you're using a group enrollment)

### 4. Uno WiFi Rev2 only: TLS certificate — try without it first

**Skip this step for now.** Upload `00_ConnectionTest` (Step 5 below) as-is first. The WiFiNINA module ships with a set of built-in trusted root certificates, and Azure's current one (DigiCert Global Root G2) is common enough across the internet that many boards already trust it out of the box — in which case this step is unnecessary. You'll know immediately: if the Serial Monitor shows successful `Published:` messages, you're done, no certificate upload needed.

**Only if that *doesn't* work** (Serial Monitor stuck at "Connecting to WiFi" forever, or DPS provisioning failing with no other obvious cause):

1. Arduino IDE → **Tools → WiFiNINA Firmware/Certificates Updater** (or **Upload SSL Root Certificates** in IDE 2.x)
2. Add domain → type `global.azure-devices-provisioning.net` → Upload
3. (You may need to add one more domain after Step 6 below — the sketch tells you which one, if IoT Central assigns you a specific hub.)

**If the upload itself fails** with a generic "Upload failed. Please try again." — this is a known Arduino IDE bug, not something wrong with your setup: the certificate upload tool needs exclusive access to the board's serial port, and it fails with exactly that unhelpful message if the **Serial Monitor or Serial Plotter is open** at the same time. Close it first, then retry the upload.

ESP32 doesn't need any of this — its root certificate is already embedded in the library.

### 5. Open an example and set up its config

**File → Examples → AzureIoT → 00_ConnectionTest** first — it needs no sensor wiring at all, just your Wi-Fi + Azure credentials, and confirms the whole connection chain works before you touch any of the numbered sensor examples. Each example ships with a `config.h` already in its folder — just open it and edit the values directly:
- `WIFI_SSID` / `WIFI_PASSWORD`
- `IOTC_ID_SCOPE` / `IOTC_DEVICE_ID` / `IOTC_DEVICE_KEY` (from Step 3)

**Heads up:** `config.h` ships with placeholder values so the example compiles immediately — no copying a file first. If you're using this library in your own separate sketch, that's all there is to it. If you're instead working on a fork of *this* repository, remember `config.h` is a normal tracked file here: don't commit it after filling in your real credentials.

### 6. Upload and check the Serial Monitor

Select **Tools → Board →** your board (e.g. **Arduino Uno WiFi Rev2**, or whichever ESP32 board you have) and the right port, then Upload. Open **Tools → Serial Monitor** at **115200 baud**. You should see:

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

**Uno WiFi Rev2 only:** if you needed the certificate step above and MQTT still won't connect after that, copy the **"Assigned hub"** hostname from Serial Monitor, go to the Certificates Updater, add that domain too, and re-upload. From then on it keeps working even if IoT Central ever reassigns you to a different hub — the sketch re-provisions on every boot. **ESP32 doesn't need this** — its embedded root certificate already covers any hub Azure assigns you.

Check IoT Central → **Devices → your device → View** — you should see the field updating every few seconds.

**If something's not working**, see [Troubleshooting](#troubleshooting) below.

---

## The examples

All six numbered examples use **Seeed Studio Grove-ecosystem sensor modules**, designed to plug into a **Grove Base Shield** sitting on top of an Arduino Uno WiFi Rev2 (each Grove module connects with a 4-pin cable, no breadboarding needed). If you're on Uno WiFi Rev2 with the Grove kit, the pin numbers below are exactly right as-is.

**If you're on ESP32** (or any board without a Grove Base Shield): these examples assume you either have Grove-to-breadboard adapter cables for your specific board, or you're substituting an equivalent non-Grove sensor and wiring it directly — in which case update the pin constant near the top of each `.ino` (e.g. `const int PIN_TEMPERATURE = A0;`) to match your actual wiring, and check whether your specific ESP32 board's analog/digital pin numbering matches these constants at all (ESP32 boards vary in which GPIOs are broken out and analog-capable). `00_ConnectionTest` needs none of this — it's the one example with no sensor hardware requirement at all.

| # | Example | Sensor | Pin |
|---|---|---|---|
| — | `00_ConnectionTest` | **None** — just proves Wi-Fi/DPS/MQTT work, no wiring needed. Start here. | — |
| 1 | `01_TemperatureSensor` | Grove Temperature (thermistor) | A0 |
| 2 | `02_RotaryAngleSensor` | Grove Rotary Angle | A1 |
| 3 | `03_SoundSensor` | Grove Sound | A2 |
| 4 | `04_TouchSensor` | Grove Touch | Digital pin 3 |
| 5 | `05_TurbiditySensor` | Grove Turbidity | A3 |
| 6 | `06_TemperatureHumiditySensor` | Grove Temp & Humidity (DHT11) | Digital pin 2 (needs one extra library — see the comment at the top of that sketch) |

Every example is the same three-part shape:
```cpp
#include <AzureIoT.h>
#include "config.h"

void setup() {
    Serial.begin(115200);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
    // connects WiFi, provisions via DPS, connects MQTT
}

void loop() {
    AzureIoT.loop();           // ALWAYS call this every loop() -- handles reconnects + sending

    float value = /* your sensor reading + math -- write whatever you want here */;
    AzureIoT.publish("myKey", value);
}
```

## Writing your own example / project

This is the point of the redesign: **the algorithm is yours.** `AzureIoT.begin(...)` and `AzureIoT.loop()` are the only calls you need for connectivity — everything else in `loop()` (which sensors to read, what math to do, when to publish what) is ordinary Arduino code, just aimed at `AzureIoT.publish(key, value)` instead of `Serial.println(...)`.

A few things worth knowing:

- **`begin()` takes your five credentials as plain arguments, not by reading `config.h` itself.** Your sketch includes its own `config.h` (which it can always see, being in the same folder) and hands the values to `AzureIoT.begin(ssid, password, idScope, deviceId, deviceKey)`. This isn't just style — a library's own source files can't reliably see a *sketch's* local files in current Arduino tooling, so the library reading `config.h` directly used to fail with a `config.h: No such file or directory` error depending on IDE version.
- **Tuning knobs (send interval, timeouts, Wi-Fi self-healing thresholds, DPS retry backoff, DPS endpoint) live in the library now, not `config.h`**, with sensible defaults. Override any of them with a setter call before `begin()` — e.g. `AzureIoT.setSendInterval(10000);` — see `AzureIoT.h` for the full list.
- **`publish()` only stages a value — it doesn't send anything by itself.** `AzureIoT.loop()` sends everything staged since the last send, as one combined JSON message, on a timer (default 5 seconds, override with `setSendInterval()`). Call `publish()` as often as you like — from a fast loop, from inside an `if`, whatever — the actual send rate stays bounded and predictable. This matters if you have several boards sharing one Wi-Fi network and one Azure app: unbounded sends per sensor reading would make the network and Azure app noisy fast.
- **Need an immediate send** instead of waiting for the timer (e.g. reacting to a button press)? Call `AzureIoT.sendNow()`.
- **`publish()`'s key must be a string literal** (`"temperature"`, not something built at runtime) — the pointer is kept, not copied.
- Publishing multiple values before a send combines them into one message: `{"temperature":24.00,"humidity":55.00}` — see Example 6, which reads both from one DHT11.
- **Want to send text instead of a number?** `AzureIoT.publishText(key, value)` sends a string value immediately (JSON-escaped for you) — unlike `publish()`, it doesn't stage/batch, so it's meant for occasional status messages and connection testing (see `00_ConnectionTest`), not high-frequency telemetry.

---

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Hangs at "Connecting to WiFi" with dots forever, then retries | Wrong `WIFI_SSID`/`WIFI_PASSWORD` in `config.h`, or board out of range |
| "DPS provisioning FAILED" | Wrong `IOTC_ID_SCOPE`/`IOTC_DEVICE_ID`/`IOTC_DEVICE_KEY`. On Uno WiFi Rev2 specifically, this can also mean the TLS certificate step (Step 4) is needed — but try without it first; many boards already trust Azure's certificate out of the box |
| Provisioning succeeds but "MQTT connect failed, rc=..." | **Uno WiFi Rev2:** you may need the *assigned hub's* domain added to the Certificates Updater too (see end of Step 6). **ESP32:** this shouldn't happen since the root cert is embedded — if it does, double-check the board actually has internet access (captive portal, firewall) |
| "Upload SSL Root Certificates" fails with "Upload failed. Please try again." | Known Arduino IDE bug, not your setup: the tool needs exclusive access to the board's serial port and fails with this exact generic message if the **Serial Monitor or Serial Plotter is open**. Close it, then retry the upload |
| Nothing appears in IoT Central even though Serial shows "Published: ..." | Check the device's template/capability model matches your published keys — a raw JSON key needs a matching capability in IoT Central for the dashboard to render it |
| "AzureIoT.publish: too many distinct keys staged" | You're publishing more than 16 differently-named keys from one sketch; combine some or raise the limit in `AzureIoT.cpp` |

---

## Long-run / unattended operation

See the top of `AzureIoT.h` for the full design. Summary: Wi-Fi trouble self-heals in two free (no Azure contact) tiers -- a Wi-Fi radio reinit after ~1 min down, a full device reset after ~5 min down. DPS provisioning failures retry with exponential backoff (1→2→4→8, capped at 15 min) instead of ever resetting, since a reset can't fix a wrong config any faster and resetting on a tight timer would risk Azure rate-limiting the device. Identical design on both boards; only the underlying mechanism differs (see `platform.h`/`reset.cpp`).

The reset uses `RSTCTRL.SWRR` on Uno WiFi Rev2 (megaAVR-0's dedicated software-reset register, chosen specifically to avoid the watchdog's documented timing-accuracy quirks on that chip) and the standard `ESP.restart()` on ESP32, which has no equivalent timing concern.

## Known limitations (read before a long unattended run)

- **Peak stack depth on Uno WiFi Rev2** during DPS provisioning is ~2.2–2.6KB on a board with 6KB total SRAM (measured with `avr-gcc -fstack-usage` against the real functions, not estimated) -- but this only happens once, at boot, before the MQTT heap buffer even exists yet. Not a concern on ESP32 (hundreds of KB of RAM). Not endurance-tested on real Uno WiFi Rev2 hardware yet.
- **`reset.cpp`'s Uno WiFi Rev2 branch is not compile-tested** in the environment this was built in -- it needs the real `atmega4809` device header, only available through Arduino's own Board Manager toolchain. Confirmed against two independent sources (the official datasheet and Arduino's own bootloader source for this chip) but not built or run for real. The ESP32 branch (`ESP.restart()`) was compiled and run against the real Arduino-ESP32 core's documented API, but likewise not on physical ESP32 hardware. Test both once on real hardware -- deliberately hang the sketch and confirm it actually resets -- before depending on either for unattended operation.
- **The self-healing above catches "stuck disconnected," not an arbitrary hang.** If something else entirely freezes the sketch (e.g. a library bug wedging mid-`loop()` with Wi-Fi otherwise fine), nothing currently recovers from that on either board.
- **`extractJsonString()`** (inside the library's `dps_client.cpp`) is a lightweight substring search, not a full JSON parser -- matches Azure's documented DPS response format but isn't robust to Azure changing that structure.
- **Uno WiFi Rev2's ATECC608 crypto chip goes unused** -- authentication here is a software-computed SAS token (symmetric key), not the hardware-backed X.509 approach the chip is designed for. That'd be a bigger redesign. (ESP32 has no equivalent hardware secure element in most variants, so this doesn't apply there.)

## What's been verified (and how)

- **Crypto core** (`sha256.c`/`b64url.c`/`sas_token.c`): checked against NIST/RFC test vectors and, separately, byte-for-byte against an independent Python (`hashlib`/`hmac`/`base64`) implementation of Azure's documented SAS-token algorithm, across 16 randomized cases. Fully portable C, compiled clean for both AVR and ARM targets.
- **API usage, both platforms**: every WiFiNINA/PubSubClient call and every ESP32 WiFi/WiFiClientSecure/Esp.h call checked against each library's actual current source (arduino-libraries/WiFiNINA and espressif/arduino-esp32, respectively) before being used.
- **Azure root CA certificate (ESP32 only)**: copied verbatim from Microsoft's official Azure IoT C SDK (`Azure/azure-iot-sdk-c`, `certs/certs.c`), not reconstructed by hand. Independently verified with `openssl x509`: both embedded certificates parse as valid, correctly self-signed root CAs, and the DigiCert Global Root G2 certificate's SHA-1 fingerprint (`DF:3C:24:F9:...`) matches an independently-sourced reference.
- **`publish()`/`loop()` staging and flush logic**: directly unit-tested -- overwrite semantics (publishing the same key twice before a send keeps only the latest value), correct clearing after a send, multiple keys combined into one message, and 50 rapid calls collapsing to one field per key (confirming the bounded send-rate design actually holds).
- **`publishText()`'s JSON escaping**: unit-tested against 9 cases (plain text, embedded quotes, backslashes, newlines/tabs/carriage returns, control characters, empty strings, and buffer-boundary behavior with a deliberately undersized buffer) -- all correct, confirmed byte-for-byte against expected output. Also verified end-to-end that the final published payload matches what a JSON parser would expect (e.g. `device online, say "hi"` correctly becomes `{"status":"device online, say \"hi\""}`, not malformed JSON).
- **Long-run escalation logic, both platforms**: the exponential-backoff arithmetic was isolated and checked in a standalone repro before being trusted inside the sketch. The full Wi-Fi-escalation and provisioning-backoff paths were then each tested end-to-end through the real, unmodified `AzureIoT.cpp` -- once against a WiFiNINA-signature mock, once against an ESP32-signature mock -- confirming identical behavior on both: the reinit fires once at the 1-minute mark, the reset fires once afterward at the 5-minute mark, and provisioning failures (with Wi-Fi otherwise fine) never trigger either one.
- **All six example sketches** compiled cleanly against mocks of the real libraries for both platforms; the full library + one example were linked and run end-to-end on both, including a live check that the ESP32 path correctly wires the embedded certificate into `WiFiClientSecure::setCACert()`.
- **Not verified on either platform**: real hardware, real Azure credentials. **Uno WiFi Rev2 additionally**: the exact `atmega4809` compiler device pack isn't available in this sandbox (only through Arduino's own Board Manager). **ESP32 additionally**: Espressif's Xtensa/RISC-V toolchain isn't available in this sandbox either -- verification there used real, source-confirmed API signatures compiled and run on x86, not the actual ESP32 toolchain. Compile and test against one real device (of whichever board family you're using) before relying on this for a fleet of devices.

---

## Third-party libraries

This library depends on two Arduino libraries, installed automatically by
Library Manager: [`WiFiNINA`](https://github.com/arduino-libraries/WiFiNINA)
(LGPL 2.1) and [`PubSubClient`](https://github.com/knolleary/pubsubclient)
(MIT). Neither is vendored into this repo -- they're pulled in as normal
Library Manager dependencies -- so this library's own MIT license isn't
affected, but it's worth knowing what you're linking against.

## License

MIT — see [LICENSE](LICENSE). Use it, modify it, embed it in your own projects, commercial or not.
