# AzureIoT — Arduino Uno WiFi Rev2 → Azure IoT Central

An Arduino library that connects a Uno WiFi Rev2 to Azure IoT Central: Wi-Fi, on-device Azure DPS provisioning, and MQTT telemetry — no PC-side tools, no editing library files, no config-table to learn. Replaces the older `firedog1024/arduino-uno-wifi-iotc` + `Azure/dps-keygen` approach (the latter is archived and its connection-string feature stopped working in 2020).

This also **replaces Module 4** of the workshop ("Mengintegrasi Sensor Groove Arduino ke Microsoft Azure"). The six example sketches under `examples/` are a 1:1 replacement for Module 4's six sensor sections, each a short, complete sketch you can read top to bottom.

---

## Start here

### 1. Install the library

Copy the `AzureIoT/` folder from this repo into your Arduino libraries folder:
- Windows/Mac/Linux: `Documents/Arduino/libraries/AzureIoT`

Restart the Arduino IDE. You should now see **File → Examples → AzureIoT** listing the six exercises.

### 2. Install the two libraries AzureIoT depends on

Arduino IDE → **Sketch → Include Library → Manage Libraries**, install:
- `WiFiNINA`
- `PubSubClient` (must be v2.8 or newer — check the version shown in Library Manager)

### 3. Get your Azure IoT Central device credentials

In your IoT Central app: **Devices → your device → Connect**. You'll need:
- **ID scope**
- **Device ID**
- **Primary key** (or a computed device key, if you're using a group enrollment)

### 4. Upload Azure's TLS certificate to your board (one-time, can't be done from the sketch)

1. Arduino IDE → **Tools → WiFiNINA Firmware/Certificates Updater**
2. Add domain → type `global.azure-devices-provisioning.net` → Upload
3. (You'll add one more domain after Step 6 below — the sketch tells you which one.)

Skipping this is the single most common reason a fresh Azure/Arduino project fails to connect.

### 5. Open an exercise and set up its config

**File → Examples → AzureIoT → 01_Sensor_Suhu_TemperatureSensor** (or whichever exercise you're doing). In that sketch's folder, copy `config.h.example` to `config.h` and fill in:
- `WIFI_SSID` / `WIFI_PASSWORD`
- `IOTC_ID_SCOPE` / `IOTC_DEVICE_ID` / `IOTC_DEVICE_KEY` (from Step 3)

**`config.h` is deliberately gitignored** — only `config.h.example`, with placeholder values, is tracked, so real credentials never end up committed.

### 6. Upload and check the Serial Monitor

Select **Tools → Board → Arduino Uno WiFi Rev2** and the right port, then Upload. Open **Tools → Serial Monitor** at **115200 baud**. You should see:

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

Copy the **"Assigned hub"** hostname, go back to the Certificates Updater from Step 4, add that domain too, and re-upload. From then on it keeps working even if IoT Central ever reassigns you to a different hub — the sketch re-provisions on every boot.

Check IoT Central → **Devices → your device → View** — you should see the field updating every few seconds.

**If something's not working**, see [Troubleshooting](#troubleshooting) below.

---

## The exercises (replaces Module 4)

| # | Exercise | Sensor | Pin |
|---|---|---|---|
| 1 | `01_Sensor_Suhu_TemperatureSensor` | Grove Temperature (thermistor) | A0 |
| 2 | `02_Sensor_Sudut_Putaran_RotaryAngleSensor` | Grove Rotary Angle | A1 |
| 3 | `03_Sensor_Bunyi_SoundSensor` | Grove Sound | A2 |
| 4 | `04_Sensor_Sentuhan_TouchSensor` | Grove Touch | Digital pin 3 |
| 5 | `05_Sensor_Kekeruhan_TurbiditySensor` | Grove Turbidity | A3 |
| 6 | `06_Sensor_Suhu_Kelembapan_HumiditySensor` | Grove Temp & Humidity (DHT11) | Digital pin 2 (needs one extra library — see the comment at the top of that sketch) |

Every exercise is the same three-part shape:
```cpp
#include <AzureIoT.h>
#include "config.h"

void setup() {
    Serial.begin(115200);
    AzureIoT.begin();          // connects WiFi, provisions via DPS, connects MQTT
}

void loop() {
    AzureIoT.loop();           // ALWAYS call this every loop() -- handles reconnects + sending

    float value = /* your sensor reading + math -- write whatever you want here */;
    AzureIoT.publish("myKey", value);
}
```

## Writing your own exercise / project

This is the point of the redesign: **the algorithm is yours.** `AzureIoT.begin()` and `AzureIoT.loop()` are the only calls you need for connectivity — everything else in `loop()` (which sensors to read, what math to do, when to publish what) is ordinary Arduino code, exactly like Modules 2 and 4 already taught, just aimed at `AzureIoT.publish(key, value)` instead of `Serial.println(...)`.

A few things worth knowing:

- **`publish()` only stages a value — it doesn't send anything by itself.** `AzureIoT.loop()` sends everything staged since the last send, as one combined JSON message, on a timer (`SEND_INTERVAL_MS` in `config.h`, default 5 seconds). Call `publish()` as often as you like — from a fast loop, from inside an `if`, whatever — the actual send rate stays bounded and predictable. This matters with ~20 boards sharing one classroom Wi-Fi network and one Azure app: unbounded sends per sensor reading would make the network and Azure app noisy fast.
- **Need an immediate send** instead of waiting for the timer (e.g. reacting to a button press)? Call `AzureIoT.sendNow()`.
- **`publish()`'s key must be a string literal** (`"temperature"`, not something built at runtime) — the pointer is kept, not copied.
- Publishing multiple values before a send combines them into one message: `{"temperature":24.00,"humidity":55.00}` — see Exercise 6, which reads both from one DHT11.

---

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Hangs at "Connecting to WiFi" with dots forever, then retries | Wrong `WIFI_SSID`/`WIFI_PASSWORD` in `config.h`, or board out of range |
| "DPS provisioning FAILED" | Wrong `IOTC_ID_SCOPE`/`IOTC_DEVICE_ID`/`IOTC_DEVICE_KEY`, **or** you haven't done Step 4 (TLS certificate) yet |
| Provisioning succeeds but "MQTT connect failed, rc=..." | You haven't added the *assigned hub's* domain to the Certificates Updater yet (see end of Step 6) |
| Nothing appears in IoT Central even though Serial shows "Published: ..." | Check the device's template/capability model matches your published keys — a raw JSON key needs a matching capability in IoT Central for the dashboard to render it |
| "AzureIoT.publish: too many distinct keys staged" | You're publishing more than 16 differently-named keys from one sketch; combine some or raise the limit in `AzureIoT.cpp` |

---

## Known limitations (read before a long unattended run)

- **Peak stack depth** during DPS provisioning is roughly 1.2–1.5KB on a board with 6KB total SRAM. Verified to compile and run correctly in simulation, but not endurance-tested on real hardware yet.
- **No watchdog timer.** If the board ever hangs, it stays hung until manually power-cycled.
- **`extractJsonString()`** (inside the library's `dps_client.cpp`) is a lightweight substring search, not a full JSON parser — matches Azure's documented DPS response format but isn't robust to Azure changing that structure.
- The board's **ATECC608 crypto chip goes unused** — authentication here is a software-computed SAS token (symmetric key), not the hardware-backed X.509 approach the chip is designed for. That'd be a bigger redesign.

## What's been verified (and how)

- **Crypto core** (`sha256.c`/`b64url.c`/`sas_token.c`): checked against NIST/RFC test vectors and, separately, byte-for-byte against an independent Python (`hashlib`/`hmac`/`base64`) implementation of Azure's documented SAS-token algorithm, across 16 randomized cases.
- **API usage**: every WiFiNINA/PubSubClient call checked against the actual current library source.
- **`publish()`/`loop()` staging and flush logic**: directly unit-tested — overwrite semantics (publishing the same key twice before a send keeps only the latest value), correct clearing after a send, multiple keys combined into one message, and 50 rapid calls collapsing to one field per key (confirming the bounded send-rate design actually holds).
- **All six example sketches** compiled cleanly against mocks of the real WiFiNINA/PubSubClient/DHT libraries; the full library + one example were linked and run end-to-end.
- **Not verified**: real hardware, real Azure credentials, and the exact `atmega4809` compiler device pack (only available through Arduino's own Board Manager download, unreachable from the sandbox this was built in). Compile and test against one real device before relying on it for a group of students.
