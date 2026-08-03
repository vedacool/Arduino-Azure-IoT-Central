# AzureIoT — Step-by-Step Tutorial

**New here? This is the page to follow.** It takes you from an empty desk to a
board sending live data to the cloud — and then a handful of guided projects to
learn from. No prior Arduino or cloud experience assumed. When you finish, the
[README](README.md) is the reference for going further.

> ⏱️ **Time:** about 45–60 minutes for your first success (most of it is one-time
> setup). Later projects are 5–10 minutes each.

---

## What you'll build

A small board (Arduino or ESP32) that connects to your Wi-Fi and talks to
**Microsoft Azure IoT Central** (a website that shows your device's data on a
live dashboard). You'll do all **three** things this library can do:

1. **Send readings up** to the cloud (📤 device → cloud),
2. **Let the dashboard control the board** — flip a switch on the website, an LED
   obeys (📥 cloud → device), and
3. **Have one board react to another board's data** pulled from the cloud (🔁).

Each is one function call. Everything in between is ordinary Arduino code.

## A few words, in plain English

You'll see these terms a lot. You don't need to *understand* them to succeed —
here's just enough:

| Term | In one sentence |
|---|---|
| **Azure IoT Central** | A Microsoft website where your device shows up and its data is displayed on a dashboard. |
| **Telemetry** | The readings your device sends up (temperature, a button press, etc.). |
| **Provisioning (DPS)** | The board introducing itself to Azure at power-on so Azure knows who it is. This library does it for you. |
| **MQTT** | The messaging method the board uses to send data. Also automatic. |
| **Writable property** | A switch on the dashboard that controls something on the device (like an LED). |
| **Credentials** | Three short codes (ID scope, Device ID, Primary key) that let *your* board into *your* Azure app. You'll copy these in Part B. |

The whole point of this library: **you never program any of that plumbing.**
Two function calls (`begin()` and `loop()`) handle it. You just write "read this
sensor, send this number."

---

## What you need — checklist

- [ ] A **computer** (Windows, Mac, or Linux) with internet.
- [ ] One supported **board**:
  - **Arduino Uno WiFi Rev2**, or
  - an **ESP32** dev board (e.g. ESP32 DevKit / WROOM-32).
- [ ] A **USB cable** that fits your board (and that carries data, not just power).
- [ ] A **Microsoft account** (a free personal Outlook/Hotmail/Xbox account works — you'll make an Azure one in Part B if needed).
- [ ] *(For the sensor/LED projects, not the first test)* the **Grove sensor kit** if you have one, or the equivalent parts + jumper wires.

That's it. The very first milestone (`00_ConnectionTest`) needs **no wiring at
all** — just the board and USB.

---

# Part A — Set up the Arduino software

### A1. Install the Arduino IDE
Download and install the **Arduino IDE** (version 2.x) from
[arduino.cc/en/software](https://www.arduino.cc/en/software). Open it once so it
finishes setting up.

### A2. Add support for your board
- **ESP32:** **Tools → Board → Boards Manager**, search **esp32**, install the
  one **by Espressif Systems**. (If it doesn't appear: **File → Preferences →
  Additional Board Manager URLs**, paste
  `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`,
  then search again.)
- **Arduino Uno WiFi Rev2:** **Tools → Board → Boards Manager**, search
  **megaAVR**, install **Arduino megaAVR Boards**.

### A3. Install this library
**Sketch → Include Library → Manage Libraries…**, type **AzureIoT** in the
search box, and click **Install**. This is the easy way — the library is
published in the Arduino Library Manager.

When it installs, the IDE will likely ask **"Install missing dependencies?"** —
click **Install All**. That pulls in the two libraries it needs (PubSubClient,
and WiFiNINA for the Uno) automatically, so you can skip A4.

> *Alternative, only if you need it:* if you want a version newer than the one in
> Library Manager, or you're offline, use **Sketch → Include Library → Add .ZIP
> Library…** and pick the ZIP from the green **Code → Download ZIP** button on
> GitHub.

Restart the IDE. You should now see **File → Examples → AzureIoT**.

### A4. Dependencies (only if the IDE didn't offer them in A3)
If you *didn't* get the "Install missing dependencies?" prompt, add them by hand
via **Sketch → Include Library → Manage Libraries**:
- **PubSubClient** (v2.8 or newer) — *both* boards need this.
- **WiFiNINA** — *only* the Arduino Uno WiFi Rev2 needs this (ESP32 has Wi-Fi built in).

✅ **Checkpoint:** the IDE opens, your board appears under **Tools → Board**, and
**File → Examples → AzureIoT** lists the examples.

---

# Part B — Set up Azure IoT Central

This is the part most people miss — the board can't send data until there's a
cloud app and a device waiting for it. Do it once.

> ℹ️ **Heads-up (2026):** Microsoft's plans for IoT Central have been publicly
> uncertain (a 2027 retirement was announced in 2024, then that message was
> retracted). Creating apps still works today, but the screens below may look a
> little different over time — follow the *equivalent* buttons if a label has
> moved.

### B1. Sign in
Go to **[apps.azureiotcentral.com](https://apps.azureiotcentral.com)** and sign
in with your Microsoft account.

### B2. Create an application
1. Click **Build** (or **+ New application**).
2. Choose **Custom application**.
3. Give it an **Application name** (e.g. `my-first-iot`) — this also sets its web
   address (`my-first-iot.azureiotcentral.com`; **write this subdomain down**,
   you'll need it for the advanced Example 21 later).
4. Pick the **Free** pricing plan to start.
5. Click **Create**. You now have your own IoT dashboard.

### B3. Create a device template
A *template* tells the dashboard what data to expect and gives you the on/off
switch for the LED projects.

1. In your app: **Device templates → + New → IoT device →** Next.
2. Name it (e.g. `Grove Board`) → **Create**.
3. Click **+ Add capability** (or **Add inherited interface → custom**) and add:
   - A **Telemetry** named exactly **`temperature`**, type **Double**. *(Matches the sensor examples.)*
   - A **Property** named exactly **`ledState`**, type **Boolean**, and mark it
     **Writable**. *(This is the dashboard switch for the LED examples.)*
4. Click **Publish** (top bar) and confirm. **A template must be published before
   devices can use it.**

> **Capability cheat-sheet — add these as you try more examples.** Each raw key a
> sketch publishes needs a matching capability in this template (then **Publish**
> the template again) to show on the dashboard.
>
> Add as **Telemetry → Double** (the on/off ones read 1/0, so Integer is fine too):
> `touch` (01) · `button` (02, 20) · `water` (03, 14) · `sound` (04) ·
> `light` (05, 18) · `moisture` (06) · `rotaryAngle` (07) · `turbidity` (08) ·
> `temperature` (09, 10, 19) · `humidity` (10, 19) · `angle` (13) ·
> **`distance` (22)** · **`gas` (23)** · **`motion` (24)**.
>
> Add as **Property → Writable → Boolean** (the dashboard switches):
> `ledState` (15, 17, 18, 20) · `buzzerOn` (16) · `muted` (19).

### B4. Create a device and get your credentials
1. **Devices → + New**.
2. Set **Device template** to the one you just published (`Grove Board`).
3. Give a **Device name** and a **Device ID** (e.g. `arduino1`, lowercase, no spaces).
4. **Create**, then click the device → **Connect** (top-right).
5. A panel shows three values — **copy all three**, you'll paste them in Part C:
   - **ID scope**
   - **Device ID**
   - **Primary key**

✅ **Checkpoint:** you have an IoT Central app, a *published* template, a device,
and its three credentials copied somewhere safe.

---

# Part C — First success: send data (no wiring)

### C1. Open the connection test
**File → Examples → AzureIoT → 00_ConnectionTest**.

### C2. Fill in `config.h`
Click the **`config.h`** tab at the top of the sketch window. Replace the
placeholders with your real values:

```cpp
static const char WIFI_SSID[]      = "your-wifi-name";
static const char WIFI_PASSWORD[]  = "your-wifi-password";
static const char IOTC_ID_SCOPE[]  = "paste ID scope from Part B4";
static const char IOTC_DEVICE_ID[] = "paste Device ID from Part B4";
static const char IOTC_DEVICE_KEY[]= "paste Primary key from Part B4";
```

> ⚠️ Wi-Fi note: most of these boards only join **2.4 GHz** Wi-Fi, not 5 GHz. If
> your network has separate names, use the 2.4 GHz one. If your password has a
> `\` or `"` in it, put a `\` in front of it (`\\` and `\"`).

### C3. Upload
1. **Tools → Board →** select your exact board.
2. **Tools → Port →** select the port that appears when the board is plugged in.
3. Click **Upload** (→ arrow). Wait for "Done uploading."

### C4. Watch it work
Open **Tools → Serial Monitor** and set the baud rate (bottom-right) to
**115200**. You should see, over ~10–20 seconds:

```
Connecting to WiFi: ...
WiFi connected, IP: ...
Waiting for network time...
Provisioning via DPS...
Assigned hub: iotc-....azure-devices.net
Connecting to MQTT as ...
MQTT connected.
Published: {"message":"Hello from AzureIoT, message #1"}
```

Then open your IoT Central app → **Devices → your device → Raw data** — the
messages appear every few seconds. 🎉 **That's the whole chain working.**

> 📦 **Arduino Uno WiFi Rev2 only — if it gets stuck** at "Connecting to WiFi"
> forever or DPS keeps failing, you may need a one-time certificate upload.
> **First close every Serial Monitor window** (the uploader needs the port to
> itself), then **Tools → WiFiNINA Firmware/Certificates Updater → Add New →**
> type `global.azure-devices-provisioning.net:443` → select your board → Upload.
> ESP32 never needs this. Full details are in the [README troubleshooting](README.md#troubleshooting).

---

# Part D — The three modes

This library does exactly **three** things, and the examples teach them in that
order. Work one example from each mode and you've seen the whole picture. Every
project has the same shape: **open the example → set `config.h` (same Wi-Fi +
Azure values as Part C) → wire the part → upload → watch.**

| Mode | Direction | The one call | Example to start with |
|---|---|---|---|
| **1** | Device → Cloud (send readings up) 📤 | `AzureIoT.publish()` | `01_TouchSensor` |
| **2** | Cloud → Device (dashboard controls the board) 📥 | `AzureIoT.onBoolProperty()` | `15_LedCloudControl` ⭐ |
| **3** | Azure data → Device reacts (react to *another* device) 🔁 | `AzureIoT.onRemoteTelemetry()` | `21_RemoteTemperatureAlarm` |

> 🔌 **Wiring note.** The examples are built for **Grove modules on a Grove Base
> Shield** on the Uno — just plug the module into the labeled port. On **ESP32**
> there's no shield, so wire the module's signal pin to the GPIO the sketch uses
> (shown per project), plus the module's **VCC→3V3** and **GND→GND**.

---

## Mode 1 — Device → Cloud: send readings up 📤

The board reads something and sends the number to Azure with
`AzureIoT.publish("key", value)`. You already did the simplest version (a text
message) in Part C.

**Do this one — `01_TouchSensor`:**
- **Wire:** Grove Touch → **D3** (Uno) / **GPIO 15** (ESP32, picked automatically).
- **See:** Serial shows `Published: {"touch":1}` when you touch it, `0` otherwise.
- **Show it on the dashboard:** in IoT Central, add a **Telemetry** named `touch`
  to your template (same as Part B3) and **Publish** — the value now appears on
  the device page.

**Same idea, more to explore:** `02_ButtonSensor` and `24_PirMotion` (digital in);
`22_UltrasonicDistance` (single-pin distance, no extra library to install); the
analog sensors `04`–`09` and `23_GasSensor` (sound, light, temperature, gas…);
`11`–`14`, which drive an LED/buzzer *and report their own state up*. All Mode 1
— all just `publish()`.
> ⚠️ **Analog sensors on ESP32** (`04`–`09`, `13`, `17`, `18`, `23`): these now
> auto-select a valid ESP32 ADC1 pin (some Grove names like `A1`/`A2` don't exist
> on the ESP32 core and wouldn't even compile). You still need to **re-tune the
> thresholds** — ESP32's analog range is 0–4095 vs the Uno's 0–1023. See the
> [README ESP32 note](README.md#the-examples).

---

## Mode 2 — Cloud → Device: control the board from the dashboard 📥

Now the other direction: a switch on the IoT Central dashboard controls something
on the board, via `AzureIoT.onBoolProperty("name", callback)`. It uses the
**`ledState`** switch you created in Part B3.

**Do this one — `15_LedCloudControl` ⭐ (the "wow" moment):**
- **Wire:** Grove LED → **D7** (Uno) / **GPIO 2** (ESP32 onboard LED — may need no wiring).
- **Do:** upload, then open your device in IoT Central. If the switch isn't there,
  your template needs the **writable `ledState` property**, **Published** (Part B3).
- **See:** flip `ledState` on the dashboard → it shows **Pending**, the LED changes,
  then it flips to **✓ Accepted**. Serial: `matched registered property 'ledState' = true`.
- **Try:** power the board **off**, flip the switch on the dashboard, power back
  **on** — it picks up the switch's state the instant it reconnects. (That's what
  makes it a *property*, not a one-shot command.)

**Same idea, more to explore:** `16_BuzzerCloudControl` (add a `buzzerOn` writable
property to your template first). `17`/`18`/`19` combine the board's *own* logic
with cloud control. `20_ButtonLedTwoWaySync` is the hardest — a physical button
**and** the dashboard both control one LED and stay in sync.

---

## Mode 3 — Azure data → Device reacts: one board reacts to another's data 🔁

The third mode reads a **different** device's data out of Azure and reacts to it —
e.g. **Board B sounds a buzzer when Board A's temperature goes above 30 °C**,
without Board B having its own thermometer. This is
`AzureIoT.onRemoteTelemetry(...)`, in example **`21_RemoteTemperatureAlarm`**.

Two things make this mode different from the others:
- **It needs two devices.** Board A runs a Mode-1 temperature example (e.g.
  `09_TemperatureSensor`) and publishes `temperature`; Board B runs `21` and
  watches it. In a class, Board A can be a **classmate's board** — just use its
  Device ID.
- **It uses a different key and it *polls*.** Azure won't let one device subscribe
  to another's live stream (a security boundary), so Board B checks every few
  seconds using an app-wide **API token** — not a device key. While a board is in
  this mode it *only* pulls; it does **not** send its own telemetry.

**Extra one-time setup — create an API token:**
In IoT Central: **Permissions → API tokens → + New →** name it → role
**App Operator** (least-privileged role that can read telemetry) → **Generate**.
**Copy the whole token string** (it starts with `SharedAccessSignature ...`) —
you only get to see it once.

**Configure `21` on Board B:**
- In its **`config.h`** set your Wi-Fi, plus:
  - `IOTC_REMOTE_APP_SUBDOMAIN` — your app's subdomain (the `myapp` in
    `myapp.azureiotcentral.com`, from Part B2)
  - `IOTC_REMOTE_API_TOKEN` — the full token you just copied
  - *(leave `IOTC_ID_SCOPE`/`IOTC_DEVICE_ID`/`IOTC_DEVICE_KEY` untouched — a
    pull-only board never uses them)*
- In `21_RemoteTemperatureAlarm.ino`, set `REMOTE_DEVICE_ID` to **Board A's** Device ID.
- **Wire:** Buzzer → **D6** (Uno) / **GPIO 4** (ESP32).

**See:** Board B's Serial prints `Remote temperature: 24.50` every ~15 s, and the
buzzer sounds once Board A's reading passes 30 °C (and stops below 29 °C).

> 💡 Because this is polling, there's a delay (up to your interval — default 15 s,
> minimum 1 s). For *instant* reactions the right tool is an IoT Central **Rule**;
> this mode is for "check periodically and react."

---

# Part E — When something doesn't work

Start with the smallest question: **did `00_ConnectionTest` work?** If not, the
problem is Wi-Fi/Azure/credentials, not your sensor.

| What you see | Most likely fix |
|---|---|
| Stuck at "Connecting to WiFi …" | Wrong Wi-Fi name/password in `config.h`, or it's a 5 GHz network (use 2.4 GHz). |
| "DPS provisioning FAILED" | Re-check the three Azure values in `config.h`. On Uno, you may need the certificate step (box in Part C). |
| Uploads fail / wrong port | Pick the right **Tools → Port**; close the Serial Monitor and try again. |
| Serial shows `Published:` but nothing on the dashboard | The telemetry name needs a matching **capability** in your template (Part B3), then **Republish**. Try a hard browser refresh. |
| The `ledState` switch isn't on the dashboard | Your template needs the **writable `ledState` property**, and must be **Published** (Part B3). |

The [README troubleshooting table](README.md#troubleshooting) has the full list.

---

# You're ready to build your own

Once a project works, open its `.ino` — it's short. The pattern is always:

```cpp
void loop() {
    AzureIoT.loop();                       // keep the connection alive (always)
    float value = /* read your sensor */;  // ordinary Arduino code
    AzureIoT.publish("myKey", value);      // send it up
}
```

Everything else — send rate, multiple values, text messages, timeouts, the
watchdog — is in the **[README reference](README.md#writing-your-own-example--project)**.
Have fun. 🚀
