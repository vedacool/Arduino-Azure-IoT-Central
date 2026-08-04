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
   you'll need it for the advanced remote-telemetry example later).
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
4. **Make the toggle actually appear (the step almost everyone misses):** in the
   template, go to **Views → Generate default views → Generate**. This builds a form
   on the device page where the writable `ledState` switch shows up. **Without a
   view, there is nowhere on the device page to flip the switch** — this is the #1
   reason the Mode 2 examples "don't work."
5. Click **Publish** (top bar) and confirm. **A template must be published before
   devices can use it** — and **re-publish every time you change it** (add a
   capability, generate views, etc.).

> **Capability cheat-sheet — add these as you try more examples.** Each raw key a
> sketch publishes needs a matching capability in this template (then **Publish**
> the template again) to show on the dashboard.
>
> Add as **Telemetry → Double** (use **Double** even for the on/off 1/0 values — the
> sketches send decimals, so an Integer schema can mismatch and hide the reading) —
> add whichever the example you're running publishes (its `.ino`/Serial shows the key):
> `touch` · `button` · `water` · `sound` · `light` · `moisture` · `rotaryAngle` ·
> `turbidity` · `temperature` · `humidity` · `angle` · `distance` · `gas` · `motion`.
>
> Add as **Property → Writable → Boolean** (the dashboard switches, used by the
> *Control From Cloud* examples): `ledState` · `buzzerOn` · `muted`.

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
**9600**. You should see, over ~10–20 seconds:

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

> 📖 **Want the full click-by-click steps for a *specific* example** (exact wiring,
> the exact IoT Central capability/property to add, and what you'll see)? The
> **[Example Cookbook → GUIDE.md](GUIDE.md)** has a complete, self-contained
> walkthrough for **all 28 examples**. This Part D teaches the *ideas* behind each
> mode; the cookbook is the per-example recipe.

| Mode | Direction | The one call | Example to start with |
|---|---|---|---|
| **1** | Device → Cloud (send readings up) 📤 | `AzureIoT.publish()` | `01_TouchSensor` |
| **2** | Cloud → Device (dashboard controls the board) 📥 | `AzureIoT.onBoolProperty()` | `01_LedCloudControl` ⭐ |
| **3** | Azure data → Device reacts (react to *another* device) 🔁 | `AzureIoT.onRemoteTelemetry()` | `01_RemoteTemperatureAlarm` |

> 🔌 **Wiring note.** The examples are built for **Grove modules on a Grove Base
> Shield** on the Uno — just plug the module into the labeled port. On **ESP32**
> there's no shield, so wire the module's signal pin to the GPIO the sketch uses
> (shown per project), plus the module's **VCC→3V3** and **GND→GND**. **On ESP32, power Grove sensors from 3V3, not 5V — ESP32 GPIOs are not 5 V-tolerant, so a 5 V sensor signal can damage the board.**

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

**Same idea, more to explore:** `02_ButtonSensor` and `17_PirMotion` (digital in);
`15_UltrasonicDistance` (single-pin distance, no extra library to install); the
analog sensors `04`–`09` and `16_GasSensor` (sound, light, temperature, gas…);
`11`–`14`, which drive an LED/buzzer *and report their own state up*. All Mode 1
— all just `publish()`.
> ⚠️ **Analog sensors on ESP32:** these auto-select a valid ESP32 ADC1 pin (some
> Grove names like `A1`/`A2` don't exist on the ESP32 core and wouldn't even
> compile). You still need to **re-tune the thresholds** — ESP32's analog range
> is 0–4095 vs the Uno's 0–1023. See the [README ESP32 note](README.md#the-examples).

---

## Mode 2 — Cloud → Device: control the board from the dashboard 📥

Now the *other* direction — a switch on the IoT Central website turns something on
the board on and off, using `AzureIoT.onBoolProperty("name", callback)`. We'll use
the **`ledState`** switch from your template (Part B3).

### `01_LedCloudControl` — step by step

1. **Check your template is ready** (Part B3). It must have: the **writable
   `ledState` Boolean property**, a **generated view**, and be **Published**. If you
   skipped *"Views → Generate default views"*, do it now and **Publish** again —
   *that view is what makes the toggle appear on the device page.*
2. **Open the example:** File → Examples → AzureIoT → **3_Control_From_Cloud →
   01_LedCloudControl**.
3. **Edit `config.h`** — the same five Wi-Fi + Azure values as Part C.
4. **Wire the LED:** Grove LED → **D7** on the Uno; on **ESP32** it uses the
   **onboard LED (GPIO 2)**, so you may not need to wire anything.
5. **Upload**, open the **Serial Monitor at 9600 baud**, and wait for `MQTT connected.`
6. **Flip the switch in IoT Central:** **Devices → your device →** open the tab/form
   that shows the `ledState` toggle (the view you generated) → turn `ledState`
   **On** → click **Save**.
7. **Watch it happen:** on the website the property goes **Pending → ✓ Accepted**,
   the **LED turns on**, and the Serial Monitor prints
   `matched registered property 'ledState' = true`. Turn it **Off** → LED off.
8. **The clever bit:** power the board **off**, flip the switch **on** on the website,
   then power the board **back on** — it comes up with the LED already on, by itself.
   That's what makes it a *property* (a remembered setting), not a one-shot command.

> 🛑 **If there's no toggle on the device page at all:** your template is missing the
> writable property, missing a generated view, or wasn't re-published. Go back to
> step 1 — this is the most common snag.

**Same idea, more to explore:**
- **`07_LedLcdCloudControl`** — same LED control, but it *also* shows **"LED: ON/OFF"
  on a Grove LCD** (needs the **"Grove - LCD RGB Backlight"** library, ≥ 1.0.2).
- **`02_BuzzerCloudControl`** — a buzzer instead of an LED (first add a `buzzerOn`
  writable property to your template, the same way you added `ledState`, and
  re-generate views + publish).
- **`03` / `04` / `05`** — the board's *own* logic (a clap, a light level, a
  temperature) **plus** a cloud override. **`06_ButtonLedTwoWaySync`** — a physical
  button **and** the dashboard both control one LED and stay in sync (the hardest).

---

## Mode 3 — Azure data → Device reacts: one board reacts to another's data 🔁

The third mode has one board read a **different** board's data out of Azure and
react to it — e.g. **Board B sounds a buzzer when Board A's temperature goes above
30 °C**, even though Board B has no thermometer of its own. This is
`AzureIoT.onRemoteTelemetry(...)`, in **`01_RemoteTemperatureAlarm`**.

> ⚠️ **This one needs TWO boards** (and two devices in IoT Central):
> - **Board A — the sender.** A normal Mode-1 board that publishes `temperature`.
>   In class this can be a **classmate's** board.
> - **Board B — the reader.** Runs `01_RemoteTemperatureAlarm`, watches Board A's
>   reading, and buzzes. **This is the board you're setting up.**

### Part 1 — get Board A sending (skip if it already is)
1. On **Board A**, upload **2_Send_To_Cloud → 09_TemperatureSensor**, set up with
   **its own** device (say, a device named `arduino1`) — exactly like Part C / Mode 1.
2. Confirm it's live: IoT Central → **Devices → Board A's device → Raw data** shows
   `temperature` arriving. **Write down Board A's Device ID** — Board B needs it.

### Part 2 — set up Board B (the reader)
3. **Create an API token** — this is a *different* credential from a device key; it
   lets a board read the whole app's telemetry. IoT Central → **Permissions → API
   tokens → + New** → name it → role **App Operator** → **Generate** → **copy the
   whole token string now** (it starts with `SharedAccessSignature ...`; you can't
   view it again later).
4. **Find your app subdomain** — the first part of the app's web address, e.g. for
   `myapp.azureiotcentral.com` the subdomain is just `myapp`.
5. **Open the example:** File → Examples → AzureIoT → **4_React_To_Another_Device →
   01_RemoteTemperatureAlarm**.
6. **Edit `config.h`:**
   - `WIFI_SSID` / `WIFI_PASSWORD` — your Wi-Fi (Board B still needs internet).
   - `IOTC_REMOTE_APP_SUBDOMAIN` — the subdomain from step 4 (just `myapp`, not the URL).
   - `IOTC_REMOTE_API_TOKEN` — the full token from step 3.
   - **Leave `IOTC_ID_SCOPE` / `IOTC_DEVICE_ID` / `IOTC_DEVICE_KEY` as their
     placeholders** — Board B never connects its *own* identity (it only reads), so
     these three are unused here.
7. **Point it at Board A:** in `01_RemoteTemperatureAlarm.ino` (near the top), set
   `REMOTE_DEVICE_ID` to **Board A's Device ID** from step 2.
8. **Wire Board B's buzzer:** Grove Buzzer → **D6** (Uno) / **GPIO 4** (ESP32).
9. **Upload Board B**, open the **Serial Monitor at 9600 baud**.

### What you'll see
Board B prints `Remote temperature: 24.50` every ~15 seconds — it's **polling** Board
A's latest value through Azure — and the buzzer **sounds once Board A's temperature
passes 30 °C** (and goes quiet below 29 °C). Warm Board A's sensor with your fingers
to push it over the line and hear it.

> 📟 **Prefer a screen to a buzzer?** `02_RemoteTemperatureLcd` does the same thing
> but shows Board A's temperature live on a **Grove LCD** (needs the
> **"Grove - LCD RGB Backlight"** library, ≥ 1.0.2).

> 💡 **Two things students always ask:**
> - *"Board B doesn't show up as sending data."* Correct — it's **pull-only**: it
>   never sends its own telemetry, it only *reads* Board A's.
> - *"There's a lag."* Yes — it **polls** (default every 15 s, minimum 1 s). For an
>   instant reaction you'd use an IoT Central **Rule**; this mode is "check and react."

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
| The `ledState` switch isn't on the device page at all | Your template needs **both** the writable `ledState` property **and a generated View** (Views → Generate default views), then **Publish**. **No view = the property exists but there's nowhere to flip it** — the usual cause (Part B3). |
| The toggle is there but nothing happens on the board | Check the Serial Monitor (9600) says `MQTT connected`, and that you clicked **Save** after flipping it. |

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
