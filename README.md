# AzureIoT — Arduino Uno WiFi Rev2 or ESP32 → Azure IoT Central

An Arduino library that connects a Uno WiFi Rev2 **or an ESP32 board** to Azure IoT Central: Wi-Fi, on-device Azure DPS provisioning, and MQTT telemetry — no PC-side tools, no editing library files, no config-table to learn.

If you just want to get a Uno WiFi Rev2 or ESP32 board talking to Azure IoT Central with a couple of function calls, skip straight to **Start here** below. The `examples/` folder has twenty-four ready-to-run sketches -- sensors, actuators, and combinations of both, including cloud-to-device control -- each a short, complete sketch you can read top to bottom, plus a connection-test sketch that needs no hardware at all.

> 🚀 **New to this, or running a workshop?** Follow the **[step-by-step Tutorial](TUTORIAL.md)** instead. It assumes no prior experience and covers the one thing this README leaves out — **creating your Azure IoT Central app, device template, and device** (the steps below assume you already have them) — plus first success and a guided set of projects, in plain language. Everything below is the **reference**: quick setup, the full example list, and the API. And for a **click-by-click walkthrough of every single example** (wiring, the exact IoT Central setup, and what you'll see), open the **[Example Cookbook → GUIDE.md](GUIDE.md)**.

---

## Start here

### 1. Install the library

**Easiest: Arduino IDE → Sketch → Include Library → Manage Libraries…**, search **AzureIoT**, and click **Install** — it's published in the Arduino Library Manager. If the IDE offers to install dependencies, accept them (`PubSubClient`, plus `WiFiNINA` on the Uno).

**Alternative (ZIP):** download this repo as a ZIP (green **Code** button → **Download ZIP** on GitHub) and use **Sketch → Include Library → Add .ZIP Library...** — handy if you want a version newer than the one currently in Library Manager.

**Alternative (manual):** clone or unzip this repo, rename the resulting folder to `AzureIoT`, and place it directly in your Arduino libraries folder (`Documents/Arduino/libraries/AzureIoT`).

Either way, restart the Arduino IDE afterward. You should now see **File → Examples → AzureIoT** listing the examples.

### 2. Board-specific setup

**If you're using an ESP32 board:**
- Install ESP32 board support if you haven't already: **Tools → Board → Boards Manager** → search "esp32" → install the one **by Espressif Systems** (there can be similarly-named third-party entries — Espressif Systems is the correct, official one). If it doesn't show up in the search, add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to **Additional Board Manager URLs** under **File → Preferences** first, then search again.
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

1. **Close every Serial Monitor and Serial Plotter window first — all of them, including any other Arduino IDE window you might have open.** This isn't optional: the certificate uploader needs exclusive access to the board's serial port, and it silently fails with a generic "Upload failed. Please try again." if *any* serial connection to the board is still open anywhere. This is a known Arduino IDE bug, not something wrong with your setup — but it fails every single time if you skip this, so do it first rather than treating it as a fallback if the upload fails.
2. Arduino IDE → **Tools → WiFiNINA Firmware/Certificates Updater** (or **Upload SSL Root Certificates** in IDE 2.x)
3. Click **Add New**, type the domain **with its port**: `global.azure-devices-provisioning.net:443` (the bare domain without `:443` may not work — always include the port) → check its box → select your board → Upload. Wait for "Certificates uploaded" before closing the window.
4. (You may need to add one more domain after Step 6 below — the sketch tells you which one, if IoT Central assigns you a specific hub. Same `:443` format.)

ESP32 doesn't need any of this — its root certificate is already embedded in the library.

### 5. Open an example and set up its config

**File → Examples → AzureIoT → 00_ConnectionTest** first — it needs no sensor wiring at all, just your Wi-Fi + Azure credentials, and confirms the whole connection chain works before you touch any of the numbered sensor examples. Each example ships with a `config.h` already in its folder — just open it and edit the values directly:
- `WIFI_SSID` / `WIFI_PASSWORD`
- `IOTC_ID_SCOPE` / `IOTC_DEVICE_ID` / `IOTC_DEVICE_KEY` (from Step 3)

**Heads up:** `config.h` ships with placeholder values so the example compiles immediately — no copying a file first. If you're using this library in your own separate sketch, that's all there is to it. If you're instead working on a fork of *this* repository, remember `config.h` is a normal tracked file here: don't commit it after filling in your real credentials.

### 6. Upload and check the Serial Monitor

Select **Tools → Board →** your board (e.g. **Arduino Uno WiFi Rev2**, or whichever ESP32 board you have) and the right port, then Upload. Open **Tools → Serial Monitor** at **9600 baud**. You should see:

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

The `examples/` folder is organised into **four groups** — they show up as submenus under **File → Examples → AzureIoT**, matching the three things this library does plus a no-hardware starting point:

1. **Start Here** — the connection test (no wiring).
2. **Send To Cloud** — device → cloud telemetry (`publish()`).
3. **Control From Cloud** — cloud → device, via writable properties (`onBoolProperty()` / `onNumberProperty()`) and commands (`onCommand()`).
4. **React To Another Device** — react to another device's telemetry (`onRemoteTelemetry()`).

Within each group the examples are numbered roughly easiest-to-hardest. The sensor/actuator examples use **Seeed Studio Grove modules** on a **Grove Base Shield** atop an Arduino Uno WiFi Rev2 — the Uno pin numbers in the tables below are exactly right for that setup.

**If you're on ESP32** (or any board without a Grove Base Shield): every example auto-selects ESP32-safe pins via an `#if defined(ARDUINO_ARCH_ESP32)` block near the top. This matters because the Grove/Uno pins are **actively unsafe** on ESP32 — **GPIO 6–11 are wired to its SPI flash** (the LED/buzzer examples' D7/D6 would crash the board), and **GPIO 3 is its Serial RX** (the touch example's D3) — so on ESP32 they default to GPIO 2 (LED), 4 (buzzer), 15 (touch), etc. The **analog** examples additionally need a valid **ADC1** pin (some Grove names like `A1`/`A2` aren't even defined on ESP32 and won't compile), and the ESP32's 12-bit ADC (0–4095, different reference) makes their Uno-calibrated thresholds/math **read wrong until you re-tune them** — see each sketch's ESP32 pin comment. Also **power Grove sensors from 3V3, not 5V, on ESP32** — its GPIOs aren't 5 V-tolerant. `00_ConnectionTest` needs none of this.

> 📟 **Three examples use a Grove LCD RGB Backlight** (one per mode — `18_ButtonLcd`, `07_LedLcdCloudControl`, `02_RemoteTemperatureLcd`). They need the extra **"Grove - LCD RGB Backlight"** library, **v1.0.2 or newer** (Library Manager) — 1.0.2 auto-detects both the v4.0 and v5.0 hardware modules (older versions only drive the v4.0 backlight). The LCD is I2C — plug it into any I2C port on the shield, no pin to set. The DHT examples (`10`, `05`) similarly need the Grove DHT library. These are documented per-sketch, not forced on everyone via `depends`.

### 1 · Start Here
| # | Example | What it demonstrates | Hardware / pin |
|---|---|---|---|
| — | `00_ConnectionTest` | **None** — proves Wi-Fi/DPS/MQTT work, no wiring. Start here. | — |

### 2 · Send To Cloud — device → cloud (telemetry)
| # | Example | What it demonstrates | Hardware / pin (Uno) |
|---|---|---|---|
| 01 | `TouchSensor` | Simplest read: digital in, publish | Grove Touch, D3 |
| 02 | `ButtonSensor` | Same shape as Touch | Grove Button, D4 |
| 03 | `WaterSensor` | Digital in, publish | Grove Water, D5 |
| 04 | `SoundSensor` | Analog in (averaged) | Grove Sound, A2 |
| 05 | `LightSensor` | Analog in | Grove Light, A4 |
| 06 | `MoistureSensor` | Analog in | Grove Moisture, A5 |
| 07 | `RotaryAngleSensor` | Analog in + voltage/degree math | Grove Rotary Angle, A1 |
| 08 | `TurbiditySensor` | Analog in + voltage conversion | Grove Turbidity, A3 |
| 09 | `TemperatureSensor` | Analog in + thermistor math | Grove Temperature, A0 |
| 10 | `TemperatureHumiditySensor` | First extra-library dependency (DHT11) | Grove Temp & Humidity, D2 |
| 11 | `LedStatusReport` | First actuator — reports its own state | Grove LED, D7 |
| 12 | `BuzzerStatusReport` | Same shape, second actuator | Grove Buzzer, D6 |
| 13 | `RotaryAngleLedDimmer` | First analog *output* (PWM) | Rotary Angle (A1) → LED (PWM D3) |
| 14 | `WaterAlarm` | One sensor drives multiple actuators | Water (D5) → Buzzer (D6) + LED (D7) |
| 15 | `UltrasonicDistance` | Single-pin ultrasonic distance, no extra library | Grove Ultrasonic Ranger, D7 |
| 16 | `GasSensor` | Analog gas/smoke level (relative, not ppm) | Grove Gas MQ2, A1 |
| 17 | `PirMotion` | Digital motion detect, publishes 1/0 | Grove PIR Motion, D2 |
| 18 | `ButtonLcd` | Button state shown on an LCD + published (needs LCD lib) | Grove Button D4 + Grove LCD (I2C) |

### 3 · Control From Cloud — cloud → device (writable properties + commands)
| # | Example | What it demonstrates | Hardware / pin (Uno) |
|---|---|---|---|
| 01 | `LedCloudControl` | First bidirectional — `onBoolProperty()` | Grove LED, D7 |
| 02 | `BuzzerCloudControl` | Same, second actuator | Grove Buzzer, D6 |
| 03 | `ClapToToggleLed` | Event-driven local logic + cloud control | Sound (A2) → LED (D7) |
| 04 | `AutoNightLight` | Sensor-threshold logic + cloud override | Light (A4) → LED (D7) |
| 05 | `ComfortAlarm` | Same pattern, harder DHT11 sensor | Temp & Humidity (D2) → Buzzer (D6) |
| 06 | `ButtonLedTwoWaySync` | Hardest: sensor + actuator + two-way sync | Button (D4) + LED (D7) |
| 07 | `LedLcdCloudControl` | Cloud toggle drives LED **and** shows state on an LCD (needs LCD lib) | Grove LED D7 + Grove LCD (I2C) |
| 08 | `BuzzerOffCommand` | **Commands** (`onCommand()`) — buttons that fire an action every press, any state | Grove Buzzer, D6 |
| 09 | `BlinkCommand` | Command **with a value** — blink N times (Integer request → `atoi`) | Grove LED, D7 |
| 10 | `LedBrightnessSetting` | **Numeric** writable property (`onNumberProperty()`) — a remembered 0–255 level | Grove LED, PWM D3 |

### 4 · React To Another Device — react to another device's telemetry
| # | Example | What it demonstrates | Hardware / pin (Uno) |
|---|---|---|---|
| 01 | `RemoteTemperatureAlarm` | Read a DIFFERENT device's telemetry via REST polling (v1.1+) | Buzzer, D6 (no local sensor) |
| 02 | `RemoteTemperatureLcd` | Show another device's temperature live on an LCD (needs LCD lib) | Grove LCD (I2C), no local sensor |

Every example is the same three-part shape:
```cpp
#include <AzureIoT.h>
#include "config.h"

void setup() {
    Serial.begin(9600);
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
- Publishing multiple values before a send combines them into one message: `{"temperature":24.00,"humidity":55.00}` — see Example 10, which reads both from one DHT11.
- **Want to send text instead of a number?** `AzureIoT.publishText(key, value)` sends a string value immediately (JSON-escaped for you) — unlike `publish()`, it doesn't stage/batch, so it's meant for occasional status messages and connection testing (see `00_ConnectionTest`), not high-frequency telemetry.

## Cloud-to-device control: writable properties

Everything above is device → cloud (telemetry). `onBoolProperty()` and `reportBoolProperty()` add the other direction: letting an IoT Central dashboard toggle control something on the device (an LED, a buzzer, anything binary), as a **writable property** rather than a one-shot command -- meaning it's persistent and survives a reconnect: if the dashboard toggle was flipped while the device was offline, the device picks up the correct state the moment it reconnects, not just on the next change.

```cpp
void onLedState(bool on) {
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
}

void setup() {
    AzureIoT.onBoolProperty("ledState", onLedState); // call BEFORE begin()
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // delivers property updates to onLedState(), same call as always
}
```

A few things worth knowing:

- **`begin()` has an optional 6th argument for your device template's model ID** — pass it as `IOTC_MODEL_ID` if you want to follow Microsoft's official IoT Plug and Play convention (an IoT Central device "should follow IoT Plug and Play conventions and send their model ID when they register"). **In practice, this turned out not to be required for writable properties to work** — real hardware testing confirmed `onBoolProperty()`/`reportBoolProperty()` both work correctly with or without it, at least when your DPS enrollment group already has a fixed device template configured (which handles template assignment administratively, independent of what the device announces). None of the examples in this repo set it by default, for exactly that reason. It's still good, documented practice to add if you're setting up a new enrollment group from scratch, or if you're relying on model-id-based automatic template assignment — but don't treat a stuck "pending" toggle as necessarily meaning you need this; check `DEVELOPMENT.md`'s bug history first, since a real, unrelated bug was the actual cause the one time this got investigated. If you do want to set it: find yours in IoT Central: your device template → **"{} Edit DTDL"** button → the top-level `"@id"` field, something like `"dtmi:yourapp:yourtemplate_xxx;1"` — add it to `config.h` as `static const char IOTC_MODEL_ID[] = "...";` and pass it as `begin()`'s 6th argument.
- **You still need to add the property to your device template in IoT Central** (Devices → your device's template → Add capability → Property → Writable, boolean), **then generate a View** (Views → Generate default views) so a toggle actually appears on the device page, **then Publish** the template. The library code alone doesn't make a control appear — and a common gotcha is stopping after "add the property": with no View, the property exists but there's **nowhere on the device page to switch it**. Re-publish after any template change.
- **`name` follows the same rule as `publish()`'s key** — pass a string literal (`"ledState"`), not something built at runtime.
- **The dashboard shows "Accepted" automatically.** After your callback returns, the library sends a full IoT Plug-and-Play-style acknowledgment (value, status, version) back to Azure — you don't write any of that yourself. (Confirmed against the real IoT Central UI: the toggle shows "Pending" until this ack arrives, then flips to "✓ Accepted".)
- **Fixed cap of 16 registrations**, same reasoning and same number as `publish()`'s staged-keys limit: this library never heap-allocates, so an unbounded list isn't an option on a 6KB-RAM board, and 16 is generous headroom for any real project without that risk.
- **If local logic (not the cloud) changes something the dashboard also controls** -- a physical button toggling an LED, a sensor threshold triggering a buzzer -- call `AzureIoT.reportBoolProperty(name, value)` to tell Azure about it, so the dashboard reflects the change instead of only showing whatever it last set itself. This sends a plain reported-property update, deliberately without the ack fields `onBoolProperty()`'s automatic response uses -- those specifically mean "responding to a particular desired-property version," which doesn't apply to a change nothing from the cloud asked for. See `03_ClapToToggleLed`, `04_AutoNightLight`, `05_ComfortAlarm`, and `06_ButtonLedTwoWaySync` (in *Control From Cloud*) for this in practice.

## Cloud-to-device control: numeric properties

`onNumberProperty()` is the numeric sibling of `onBoolProperty()` — a persistent numeric **setting** the dashboard sends down and the cloud remembers (a brightness level, a threshold, a target temperature), applied on reconnect exactly like the bool version:

```cpp
void onBrightness(float v) { analogWrite(LED_PIN, (int)v); }

void setup() {
    AzureIoT.onNumberProperty("brightness", onBrightness); // call BEFORE begin()
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}
```

In the template add a **writable property, schema Double**, then **Generate default views** + **Publish** (same as a bool toggle). The value arrives as a `float`. `reportNumberProperty(name, value)` reports a device-initiated change back — fire-and-forget (dropped if offline, unlike `reportBoolProperty`'s retry). See `10_LedBrightnessSetting`.

## Cloud-to-device control: commands

A **command** is a one-shot action triggered by a dashboard **button** (`onCommand()`). Unlike a property it carries no stored state — it fires *every* press, regardless of what happened before. That's the tool for "buzz off", "blink", "reboot": a property already set to "off" can't re-send "off" (no value change → nothing is pushed to the device), but a command always runs.

```cpp
void buzzerOff() { noTone(PIN_BUZZER); }                              // no parameter
void blink(const char *request) { int n = atoi(request); /* ... */ } // command with a value

void setup() {
    AzureIoT.onCommand("buzzerOff", buzzerOff);   // call BEFORE begin()
    AzureIoT.onCommand("blink", blink);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}
```

In the template add a **Command** capability with the matching Name — it shows as a **button**, no view to generate. For a command that carries a value, set its **Request schema to a primitive Integer**: IoT Central then delivers the bare value (`"3"`), which `atoi()` reads directly (an Object request arrives as JSON instead). The library auto-replies 200 after your handler returns. Commands need MQTT, so they don't run in pull-only mode. See `08_BuzzerOffCommand` and `09_BlinkCommand`.
- **If MQTT isn't connected when you call `reportBoolProperty()`, it's staged and retried automatically** -- `AzureIoT.loop()` catches up on any pending reports once MQTT reconnects. Only the *latest* value per property name is kept (a fixed 16-slot table, same as `publish()`'s staged keys), so if the same property changes several times while offline, only the final value gets sent once reconnected -- not a backlog of every intermediate change.

---

## Hardware watchdog: recovering from an arbitrary hang

The self-healing above only catches *"stuck disconnected."* If something else entirely freezes the sketch (a wedged library call, a bug in your own `loop()` code) with Wi-Fi otherwise fine, nothing recovers from that on its own. `AzureIoT.enableWatchdog()` closes that gap using the board's actual hardware watchdog timer.

```cpp
void setup() {
    AzureIoT.begin(...);
    AzureIoT.enableWatchdog(); // call AFTER begin(), not before -- see why below
}
```

A few things worth knowing:

- **This is opt-in, not automatic.** Enabling a watchdog is a real behavior change -- an unattended device can now reboot mid-operation if something hangs -- so it's a deliberate choice you make.
- **Call it after `begin()`, not before.** `begin()` itself blocks for as long as Wi-Fi connect + DPS provisioning takes, routinely longer than the watchdog's ~8 second period, and nothing pets the watchdog during that specific sequence. Enabling it before `begin()` risks the very first boot resetting itself mid-provisioning.
- **`AzureIoT.loop()` pets it automatically** -- as long as your own code between two `AzureIoT.loop()` calls finishes well under 8 seconds, you don't need to do anything else.

See [DEVELOPMENT.md](DEVELOPMENT.md) for the implementation details (why ~8 seconds, and why petting placement matters).

## Reading another device's telemetry (v1.1+)

Everything above is about *this* device's own data and control. Sometimes you want one device to react to a *different* device's reading -- e.g. Device B sounds a buzzer when Device A's `temperature` crosses 30°C.

**This can't work as a push, and that's a deliberate Azure security boundary, not a limitation of this library.** A device's credentials only ever grant it access to its own telemetry and its own twin -- there's no MQTT mechanism for one device to subscribe to another's data stream. The only way to bridge them is to poll Azure's REST API on a timer.

**A device can send its own data OR pull another device's data -- not both, at least not yet.** Real hardware testing found that keeping a persistent MQTT connection open *and* periodically opening a separate HTTPS connection for polling is a known-fragile combination on this board's Wi-Fi hardware (a real, independently-reported bug, not something specific to this library -- see DEVELOPMENT.md). So: if you register `onRemoteTelemetry()`, `begin()` automatically skips DPS/MQTT entirely for that device -- it only connects Wi-Fi and polls. Your device's own Connect credentials (`IOTC_ID_SCOPE`/`IOTC_DEVICE_ID`/`IOTC_DEVICE_KEY`) simply go unused in that case.

```cpp
void onRemoteTemperature(float value) {
    if (value > 30.0f) tone(PIN_BUZZER, 1000);
    else noTone(PIN_BUZZER);
}

void setup() {
    AzureIoT.setRemoteAccess(IOTC_REMOTE_APP_SUBDOMAIN, IOTC_REMOTE_API_TOKEN); // call BEFORE begin()
    AzureIoT.onRemoteTelemetry("temperature", IOTC_REMOTE_DEVICE_ID, onRemoteTemperature); // Device ID (not name), set in config.h; call BEFORE begin()
    AzureIoT.begin(...); // automatically runs in pull-only mode now (Wi-Fi + polling only)
}

void loop() {
    AzureIoT.loop(); // polls for the remote value on the interval below
}
```

See `01_RemoteTemperatureAlarm` (in *React To Another Device*) for the complete example.

A few things worth knowing:

- **This needs a different credential than everything else in this library** -- an IoT Central **API token**, not your device's own Connect credentials. Generate one in IoT Central: **Permissions → API tokens → New** → role **"App Operator"** (the least-privileged role that can still read telemetry -- don't use App Administrator or App Builder for this). This token is scoped to the whole *application*, not one device, so treat it with more care than a single device's key.
- **Fastest allowed: 1000ms**, set via `AzureIoT.setRemotePollInterval(ms)` (default 15000ms). This is a hard floor enforced in code, not just a suggestion -- go lower and it's silently clamped back up to 1000ms with a warning. The reason is different from every other tunable in this library: going too fast here doesn't just cost *this* device resources, it risks Azure's own **20 requests/second, per-application** rate limit -- shared across every caller of your app's API, not just this one device.
- **This is polling, not push** -- there's an inherent delay equal to whatever interval you set. If you need an instant reaction the moment a threshold is crossed, an IoT Central Rule with a Power Automate/Logic Apps action is the better fit; this feature is for "check periodically," not "react immediately."
- **Fixed cap of 4 remote watches**, lower than the 16 used for local properties, since each one costs a real network round-trip on every poll, not just a few bytes of RAM.
- **If you call `publish()` while in pull-only mode, it warns and does nothing** -- there's no MQTT connection to send through. This isn't a bug to work around; it's the plain consequence of the send-or-pull tradeoff above.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Hangs at "Connecting to WiFi" with dots forever, then retries | Wrong `WIFI_SSID`/`WIFI_PASSWORD` in `config.h`, or board out of range |
| "DPS provisioning FAILED" | Wrong `IOTC_ID_SCOPE`/`IOTC_DEVICE_ID`/`IOTC_DEVICE_KEY`. On Uno WiFi Rev2 specifically, this can also mean the TLS certificate step (Step 4) is needed — but try without it first; many boards already trust Azure's certificate out of the box |
| Provisioning succeeds but "MQTT connect failed, rc=..." | **Uno WiFi Rev2:** you may need the *assigned hub's* domain added to the Certificates Updater too (see end of Step 6). **ESP32:** this shouldn't happen since the root cert is embedded — if it does, double-check the board actually has internet access (captive portal, firewall) |
| "Upload SSL Root Certificates" fails with "Upload failed. Please try again." | Known Arduino IDE bug, not your setup: the tool needs exclusive access to the board's serial port and fails with this exact generic message if the **Serial Monitor or Serial Plotter is open**. Close it, then retry the upload |
| Nothing appears in IoT Central even though Serial shows "Published: ..." | Check the device's template/capability model matches your published keys — a raw JSON key needs a matching capability in IoT Central for the dashboard to render it |
| Device shows "Connected" and "Last data received" keeps updating, but Raw Data stays empty or shows old messages | Update to a version of this library from July 2026 or later — earlier versions were missing a required content-type declaration on the telemetry message, which let Azure accept the message without ever displaying it. If you're already on a fixed version, try a hard refresh of the IoT Central page (not just navigating within it) — the Raw Data table can lag behind the "Last data received" banner |
| "AzureIoT.publish: too many distinct keys staged" | You're publishing more than 16 differently-named keys from one sketch; combine some or raise the limit in `AzureIoT.cpp` |

---

## Long-run / unattended operation

Wi-Fi trouble self-heals in two tiers -- a Wi-Fi radio reinit after ~1 min down, a full device reset after ~5 min down. DPS provisioning failures retry with exponential backoff (capped at 15 min) instead of resetting. Identical design on both boards.

For implementation details, known limitations, and the full verification/testing history, see [DEVELOPMENT.md](DEVELOPMENT.md).

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

---

Contributing or curious how this was built and verified? See [DEVELOPMENT.md](DEVELOPMENT.md).
