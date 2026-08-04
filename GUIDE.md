# AzureIoT — Full Example Cookbook

A complete, click-by-click walkthrough for **every** example, grouped by the three
modes. **Each entry is self-contained** — pick any example and follow it start to
finish, no jumping around.

> 🆕 **First time?** Do the **[Tutorial](TUTORIAL.md)** first — it installs the
> Arduino IDE + this library and walks you through creating your Azure IoT Central
> **app, device, and credentials**. This cookbook then guides each individual example.

---

## Before you start — the once-only setup

You only do these once; then every example reuses them:

1. **Install the AzureIoT library** (Tutorial Part A) + its dependencies (`PubSubClient`, and `WiFiNINA` on the Uno).
2. Have an **Azure IoT Central app**, a **device**, and its **five credentials** (Tutorial Part B): `IOTC_ID_SCOPE`, `IOTC_DEVICE_ID`, `IOTC_DEVICE_KEY` (from *Devices → your device → Connect*), plus your Wi-Fi name/password. You paste these into each example's `config.h`.
3. Set the **Serial Monitor to 9600 baud** (bottom-right of the monitor window).

### The two IoT Central rules that trip everyone up 🛑
Most "it doesn't work" moments are one of these:

- **Telemetry** (a value the board *sends up*): add a **Telemetry** capability to your
  device template with the **exact** key name → **Views → Generate default views** →
  **Publish**. Without this the value never shows on the dashboard.
- **Writable property** (a switch that *controls* the board): add a **Boolean,
  Writable** property → **Views → Generate default views** → **Publish**. **The toggle
  only appears on the device page after you generate a view** — this is the single
  most common thing people miss.
- **Re-publish the template after *any* change** (new capability, generated views, …).

Each example below tells you *exactly* which capability/property to add.

---

## 0 · Set up Azure IoT Central once — then just add to it

**Use ONE device template and ONE device for the whole workshop, and keep *adding*
capabilities to that same template as you move through the examples** — don't make a
new template per example. (First-time app + device + template creation is in
[Tutorial Part B](TUTORIAL.md#part-b--set-up-azure-iot-central); come back here for
the capability details.)

Almost everything lives in **Device templates → [your template] → Model** and in
**Views**. These are the only four procedures you need.

> 📸 **Screenshots:** the slots below are placeholders — drop your own screenshots
> into a `docs/img/` folder (see [`docs/img/README.md`](docs/img/README.md) for the
> shot list) and replace each *"Screenshot slot"* line with an image, e.g.
> `![Add capability](docs/img/add-capability.png)`. For reference visuals right now,
> Microsoft's official walkthrough is here:
> [Add capabilities to a device template](https://learn.microsoft.com/azure/iot-central/core/howto-edit-device-template)
> and [Views](https://learn.microsoft.com/azure/iot-central/core/howto-set-up-template#views).

### A) Add a telemetry field (a value the board *sends up*)
1. **Device templates** → click **[your template]** (e.g. `Grove Board`).
2. Click the **model** — the row tagged **Root**, named after your template.
3. **+ Add capability**.
4. **Display name:** anything readable. **Name:** the **exact** key from the example (e.g. `touch`) — it must match the sketch character-for-character.
5. **Capability type:** **Telemetry**. **Schema:** **Double** for any number (use **String** only for text like `message`).
6. **Save.**

> 💡 **Always pick `Double` for numbers — even 0/1 values.** Every sketch sends
> decimals, so an **Integer** schema can mismatch and hide the reading under the
> device's **Unmodeled data** tab. If you don't see a **Schema** box, click the
> **˅** at the right end of the capability row to expand it.

> 📸 *Screenshot slot — `docs/img/add-capability.png`: the **+ Add capability** panel with Name, Capability type, and Schema filled in.*

### B) Add a control switch (a writable property the *dashboard sets*)
Same steps 1–4 as above, then:
5. **Capability type:** **Property**. **Schema:** **Boolean**. Tick **Writable**.
6. **Save.**

### C) Make it appear + go live — do this after ANY change
1. **Views** (left menu of the template) → **Generate default views** → **Generate**. This builds the charts for telemetry **and the form where writable switches appear**.
2. **Publish** (top bar) → confirm.
> ⚠️ Skipping **Generate default views** is the #1 reason a control switch never shows up on the device page.

> 📸 *Screenshot slot — `docs/img/generate-views-publish.png`: the **Views → Generate default views** screen (and/or the **Publish** button).*

### D) See a value / flip a switch (on the actual device)
- **Devices → [your device].** Telemetry shows on the **Overview** (chart) view and under the **Raw data** tab.
- A writable switch is on the **form view** (created by Generate default views): flip it, then click **Save** — that's what actually sends it to the board.

> 📸 *Screenshot slot — `docs/img/device-page.png`: the device page showing a telemetry chart and a writable switch with its **Save** button.*

### The master capability list — add these to your one template
Add each **once**. *Telemetry* = a value the board reports; *Property (Writable)* = a switch you control.

| Name (must match exactly) | Add as | Schema | Used by |
|---|---|---|---|
| `message` *(optional)* | Telemetry | String | Start Here 00 |
| `touch` | Telemetry | Double | Send 01 |
| `button` | Telemetry | Double | Send 02, 18, Control 06 |
| `water` | Telemetry | Double | Send 03, 14 |
| `sound` | Telemetry | Double | Send 04 |
| `light` | Telemetry | Double | Send 05, Control 04 |
| `moisture` | Telemetry | Double | Send 06 |
| `rotaryAngle` | Telemetry | Double | Send 07 |
| `turbidity` | Telemetry | Double | Send 08 |
| `temperature` | Telemetry | Double | Send 09, 10, Control 05 |
| `humidity` | Telemetry | Double | Send 10, Control 05 |
| `angle` | Telemetry | Double | Send 13 |
| `distance` | Telemetry | Double | Send 15 |
| `gas` | Telemetry | Double | Send 16 |
| `motion` | Telemetry | Double | Send 17 |
| `ledState` | **Property (Writable)** | Boolean | Control 01, 03, 04, 06, 07 |
| `buzzerOn` | **Property (Writable)** | Boolean | Control 02 |
| `muted` | Property (Writable) | Boolean | Control 05 |

> ⚠️ **One name = one type.** `ledState` and `buzzerOn` are used *both* ways — a
> **writable switch** in the Control examples and a plain **telemetry report** in some
> Send examples (11 / 12 / 14). A template can't have the same name as both Telemetry
> *and* Property, so add them as **writable Properties** (as above). Those Send
> examples still send the values — you'll just see them under the device's **Raw data**
> tab instead of as a chart tile. Everything else stays one-name-one-capability.

> 📟 The **React To Another Device** examples need **no capability on the reader
> board** — instead you make an **API token** (steps are in those examples). The
> *sender* board only needs `temperature` telemetry, already in the list above.

*Each example section below still names the exact capability it uses — use the
procedures above to add it (or add everything up front from the table).*

---

## 1 · Start Here

### Start Here · 00 — Connection Test (`00_ConnectionTest`)
**What it teaches:** How to prove your Wi-Fi and Azure IoT Central credentials work by sending a plain text message every 5 seconds, before touching any sensor.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 — no sensor, no wiring at all. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** nothing to plug in — this example uses no sensor.
- **ESP32:** nothing to plug in — this example uses no sensor.
**Set up in IoT Central (one-time):** **Nothing to add** — this sketch sends a text line via `AzureIoT.publishText("message", ...)`, so just open **Devices → [your device] → Raw data** and watch the messages arrive. *(Optional, to show it as a named field: **Device templates → open your template → its model → + Add capability → Telemetry** named exactly **`message`**, Schema **String** → Save, then **Views → Generate default views** → **Publish**.)*
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 1_Start_Here → 00_ConnectionTest**.
2. Click the **`config.h`** tab and fill in the five values: `WIFI_SSID`, `WIFI_PASSWORD`, `IOTC_ID_SCOPE`, `IOTC_DEVICE_ID`, `IOTC_DEVICE_KEY` (from IoT Central: Devices > your device > Connect).
3. **Tools → Board** + **Port**, then **Upload**. Open **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the Serial Monitor prints `Connected! Sending a text message every 5 seconds...`, and in IoT Central the message text (for example `Hello from AzureIoT, message #1`, then `#2`, `#3`...) updates every 5 seconds under the device's **Raw data** tab.
**Try:** watch the number at the end of the message climb by 1 every 5 seconds — that counter proves fresh messages are getting through, not just one stale value.

---

## 2 · Send To Cloud 📤 — the board sends readings up (`publish()`)

### Send To Cloud · 01 — Touch Sensor (`01_TouchSensor`)
**What it teaches:** How to read a Grove Touch Sensor (touched or not) and send that 1/0 value to the cloud.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Touch Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Touch Sensor into port **D3**.
- **ESP32:** **GPIO 15** (selected automatically). Power it from 3V3, not 5V — ESP32 isn't 5V-tolerant.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`touch`**, type **Double** → Save. Then **Views → Generate default views**, then **Publish**. The value then shows on the device's page.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 01_TouchSensor**.
2. Click the **`config.h`** tab and fill in the five values.
3. **Tools → Board** + **Port**, then **Upload**. Open **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** after `MQTT connected.`, the `touch` telemetry on the device's page reads **1** while you're touching the sensor and **0** when you let go.
**Try:** hold your finger on the pad and watch the value hold at 1, then release and watch it drop to 0 within a few seconds.

### Send To Cloud · 02 — Button (`02_ButtonSensor`)
**What it teaches:** How to read a Grove Button (pressed or not) and send 1 while pressed, 0 while released.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Button. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Button into port **D4**.
- **ESP32:** **GPIO 4** (selected automatically — the sketch uses digital pin 4 on both boards). Power it from 3V3, not 5V — ESP32 isn't 5V-tolerant.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`button`**, type **Double** → Save. Then **Views → Generate default views**, then **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 02_ButtonSensor**.
2. Click the **`config.h`** tab and fill in the five values.
3. **Upload**, open **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the `button` telemetry reads **1** while the button is held and **0** when released.
**Try:** press and hold for a few seconds and watch the dashboard sit at 1, then release and see it return to 0.

### Send To Cloud · 03 — Water Sensor (`03_WaterSensor`)
**What it teaches:** How to read a Grove Water Sensor and send 1 when water is detected, 0 otherwise.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Water Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Water Sensor into port **D5**.
- **ESP32:** **GPIO 5** (selected automatically). Power it from 3V3, not 5V — ESP32 isn't 5V-tolerant.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`water`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 03_WaterSensor**.
2. Fill in `config.h`; **Upload**; open **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the `water` telemetry reads **1** when the sensor's traces are wet and **0** when dry.
**Try:** brush a wet fingertip across the metal traces — watch it jump to 1, then dry it and watch it fall to 0.

### Send To Cloud · 04 — Sound Sensor (`04_SoundSensor`)
**What it teaches:** How to read a Grove Sound Sensor (a smoothed, 32-sample-averaged loudness number) and send it to the cloud.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Sound Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Sound Sensor into port **A2**.
- **ESP32:** **GPIO 34** (selected automatically). Power from 3V3, not 5V. ESP32's ADC is 12-bit; the math is Uno-calibrated, so values read differently — re-tune thresholds.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`sound`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 04_SoundSensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the `sound` value is small in a quiet room and jumps higher with noise.
**Try:** clap or talk loudly near the sensor and watch `sound` spike, then go quiet and watch it settle.

### Send To Cloud · 05 — Light Sensor (`05_LightSensor`)
**What it teaches:** How to read a Grove Light Sensor (photoresistor) and send the raw brightness reading (0–1023, higher = brighter).
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Light Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Light Sensor into port **A4**.
- **ESP32:** **GPIO 35** (selected automatically). Power from 3V3, not 5V. ESP32's ADC is 12-bit; re-tune thresholds.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`light`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 05_LightSensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** `light` is higher in bright light, lower in shade.
**Try:** cup your hand over the sensor (value drops), then shine a phone torch on it (climbs toward 1023).

### Send To Cloud · 06 — Moisture Sensor (`06_MoistureSensor`)
**What it teaches:** How to read a Grove Moisture Sensor and send the raw reading (0–1023, higher = wetter).
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Moisture Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Moisture Sensor into port **A5**.
- **ESP32:** **GPIO 32** (selected automatically). Power from 3V3, not 5V. ESP32's ADC is 12-bit; re-tune thresholds.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`moisture`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 06_MoistureSensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** `moisture` is low in dry air, higher in damp soil/water. Don't push the sensor in past its "highest position" line.
**Try:** note the value in open air, then dip just the tip in a cup of water and watch it rise.

### Send To Cloud · 07 — Rotary Angle Sensor (`07_RotaryAngleSensor`)
**What it teaches:** How to read a Grove Rotary Angle Sensor (a knob) and send its position converted to degrees.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Rotary Angle Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Rotary Angle Sensor into port **A1**.
- **ESP32:** **GPIO 33** (selected automatically). Power from 3V3, not 5V. ESP32's ADC is 12-bit; re-tune thresholds.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`rotaryAngle`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 07_RotaryAngleSensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** `rotaryAngle` shows degrees (roughly 0 to 300) that change as you turn the knob.
**Try:** turn the knob slowly end to end and watch it sweep from ~0 up toward 300.

### Send To Cloud · 08 — Turbidity Sensor (`08_TurbiditySensor`)
**What it teaches:** How to read a Grove Turbidity Sensor (water cloudiness, as a voltage) and send it.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Turbidity Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Turbidity Sensor into port **A3**.
- **ESP32:** **GPIO 39** (selected automatically). Power from 3V3, not 5V. ESP32's ADC is 12-bit; re-tune thresholds.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`turbidity`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 08_TurbiditySensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** `turbidity` shows a voltage-style number (~0 to 5), higher in clear water, lower in cloudy.
**Try:** read a glass of clean water, then stir in a pinch of flour/dirt and watch the number change.

### Send To Cloud · 09 — Temperature Sensor (`09_TemperatureSensor`)
**What it teaches:** How to read a Grove Temperature Sensor (thermistor), convert to °C, and send it.
**Hardware:** Arduino Uno WiFi Rev2 or ESP32 + Grove Temperature Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Temperature Sensor into port **A0**.
- **ESP32:** **GPIO 36** (selected automatically). Power from 3V3, not 5V. ESP32's ADC is 12-bit; re-tune the thermistor math.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`temperature`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 09_TemperatureSensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** `temperature` shows room temperature in °C (roughly 20–30 in a typical room).
**Try:** pinch the small sensor bead for a few seconds and watch it rise, then let go and watch it drift back.

### Send To Cloud · 10 — Temperature & Humidity Sensor (`10_TemperatureHumiditySensor`)
**What it teaches:** How to read a DHT11 temperature/humidity sensor and stream both readings to Azure as telemetry.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Temperature & Humidity Sensor (DHT11). **Extra library:** "Grove Temperature And Humidity Sensor" — install via Library Manager.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the DHT11 module into **D2**.
- **ESP32:** GPIO 2 (selected automatically). Power it from 3V3, not 5V — ESP32 isn't 5V-tolerant.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`humidity`** (Double) → Save; **+ Add capability → Telemetry** named exactly **`temperature`** (Double) → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 10_TemperatureHumiditySensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the `temperature` and `humidity` tiles on your dashboard update (this sketch publishes silently, so watch the dashboard rather than the Serial Monitor).
**Try:** breathe on the sensor and watch `humidity` jump on the dashboard.

### Send To Cloud · 11 — LED Status Report (`11_LedStatusReport`)
**What it teaches:** How a device can report an actuator's own on/off state to the cloud as plain telemetry (no cloud control yet).
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove LED. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Grove LED into **D7**.
- **ESP32:** GPIO 2 (the onboard LED, selected automatically) — no external LED needed.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`ledState`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**. *(Note: here `ledState` is plain telemetry — a report — not a writable switch.)*
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 11_LedStatusReport**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the LED blinking once a second, and the `ledState` tile toggling 0/1 in step with it.
**Try:** change `BLINK_INTERVAL_MS` to `250` to blink faster and watch the value update more often.

### Send To Cloud · 12 — Buzzer Status Report (`12_BuzzerStatusReport`)
**What it teaches:** The same "report an actuator's own state" pattern as example 11, but with a buzzer beeping on a timer.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Buzzer. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Grove Buzzer into **D6**.
- **ESP32:** GPIO 4 (selected automatically).
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`buzzerOn`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**. *(Plain telemetry, not a switch.)*
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 12_BuzzerStatusReport**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the buzzer beeping once a second, and the `buzzerOn` tile toggling 0/1 in step.
**Try:** raise `BEEP_INTERVAL_MS` to `3000` for a slower beep and watch the tile update every 3 s.

### Send To Cloud · 13 — Rotary Angle → LED Dimmer (`13_RotaryAngleLedDimmer`)
**What it teaches:** How to use analog OUTPUT — a rotary knob dims an LED locally via PWM while the angle is published as telemetry.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Rotary Angle Sensor + Grove LED. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** Rotary Angle Sensor into **A1**, LED into **D3**.
- **ESP32:** rotary on GPIO 34 (ADC1), LED on GPIO 2 (selected automatically). ESP32 ADC is 12-bit; the Uno-calibrated `/1023` math reads differently — re-tune.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`angle`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 13_RotaryAngleLedDimmer**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the LED brighten/dim as you turn the knob, and the `angle` tile tracking the degrees (0–300).
**Try:** turn slowly end to end and watch the `angle` graph sweep from ~0 to 300.

### Send To Cloud · 14 — Water Alarm (`14_WaterAlarm`)
**What it teaches:** How one sensor reading can drive two actuators at once (buzzer + LED), with all three states published as telemetry.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Water Sensor + Grove Buzzer + Grove LED. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** Water Sensor into **D5**, Buzzer into **D6**, LED into **D7**.
- **ESP32:** Water on GPIO 5, Buzzer on GPIO 4, LED on GPIO 2 (selected automatically). Power the water sensor from 3V3, not 5V.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: add three **Telemetry** capabilities, type **Double**, named exactly **`water`**, **`ledState`**, **`buzzerOn`** → Save each. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 14_WaterAlarm**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** wet the sensor → LED lights, buzzer sounds, and `water`/`ledState`/`buzzerOn` all flip to 1 together.
**Try:** dry the sensor and confirm all three tiles drop back to 0.

### Send To Cloud · 15 — Ultrasonic Distance (`15_UltrasonicDistance`)
**What it teaches:** How to read a single-wire ultrasonic ranger and publish the measured distance in centimetres.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Ultrasonic Ranger. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Ultrasonic Ranger into **D7**.
- **ESP32:** GPIO 18 (selected automatically). Power it from 3V3, not 5V.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`distance`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 15_UltrasonicDistance**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the `distance` tile following your hand as you move it toward/away (0 = nothing in range within the timeout).
**Try:** hold your hand at a fixed distance, read the value, then move it twice as far and confirm the number roughly doubles.

### Send To Cloud · 16 — Gas Sensor (`16_GasSensor`)
**What it teaches:** How to read an analog MQ2 gas/smoke sensor and publish its output voltage as a relative (uncalibrated) reading.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Gas Sensor (MQ2). **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Gas Sensor into **A1**.
- **ESP32:** GPIO 34 (ADC1, selected automatically). ESP32 ADC is 12-bit; the Uno-calibrated `5.0/1023` math reads differently — change the divisor to `4095.0`, reference to `3.3`, and re-tune. Power from 3V3, not 5V.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`gas`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 16_GasSensor**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the `gas` tile settling to a baseline after the ~20–30 s heater warm-up (ignore the first few readings), then rising when combustible gas or smoke is near.
**Try:** after warm-up, hold a just-blown-out match nearby and watch `gas` spike, then fall as the air clears.

### Send To Cloud · 17 — PIR Motion Sensor (`17_PirMotion`)
**What it teaches:** How to read a digital PIR motion sensor and publish 1 for motion / 0 for still.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove PIR Motion Sensor. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the PIR Motion Sensor into **D2**.
- **ESP32:** GPIO 14 (selected automatically). Power it from 3V3, not 5V.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`motion`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 17_PirMotion**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the `motion` tile jump to 1 when you wave a hand in front of the sensor and drop back to 0 when still.
**Try:** stand very still (falls to 0), then move (snaps to 1).

### Send To Cloud · 18 — Button + LCD (`18_ButtonLcd`)
**What it teaches:** How to add a local I2C LCD for on-device feedback while still publishing the button state to the cloud.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Button + Grove LCD RGB Backlight (I2C). **Extra library:** "Grove - LCD RGB Backlight" v1.0.2+ — install via Library Manager.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Grove Button into **D4**; plug the LCD into any I2C port.
- **ESP32:** button on GPIO 27 (selected automatically). The LCD is I2C — plug into any I2C port; no pin to set.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Telemetry** named exactly **`button`**, type **Double** → Save. Then **Views → Generate default views** → **Publish**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 2_Send_To_Cloud → 18_ButtonLcd**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**You'll see:** the LCD shows `Button:` on the top row with `Released` (dim white backlight) or `Pressed` (green backlight) below, and the `button` tile flips to 1 while you hold the button.
**Try:** press and hold — backlight turns green, tile reads 1; release — dim white, 0.

---

## 3 · Control From Cloud 📥 — the dashboard controls the board (`onBoolProperty()`)

> Every example here uses a **writable property**. Remember: after adding the property
> you **must** do **Views → Generate default views → Publish**, or no toggle appears on
> the device page.

### Control From Cloud · 01 — LED Cloud Control (`01_LedCloudControl`)
**What it teaches:** The core Mode-2 pattern — a writable Boolean property on the dashboard turns a physical LED on and off, with no local logic deciding.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove LED. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Grove LED into **D7**.
- **ESP32:** **GPIO 2** (the onboard LED, selected automatically — GPIO 7 is a flash pin and is never usable).
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Property** named exactly **`ledState`**, type **Boolean**, tick **Writable** → **Save**. Then **Views → Generate default views** (THIS is what makes the toggle appear — skip it and there is no switch), then **Publish**. On the device page, flip **`ledState`** and click **Save**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 3_Control_From_Cloud → 01_LedCloudControl**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**Use it:** IoT Central → **Devices → your device →** open the generated view → flip **`ledState`** **On** → click **Save**.
**You'll see:** `matched registered property 'ledState' = true` in Serial, the dashboard toggle move **Pending → Accepted**, and the LED light up. Flip **Off** + **Save** → `= false`, LED dark.
**Try:** set the toggle **On** while the board is unplugged, click **Save**, then power it up — it picks up the pending value on connect and the LED comes on by itself.

### Control From Cloud · 02 — Buzzer Cloud Control (`02_BuzzerCloudControl`)
**What it teaches:** The same writable-property control drives a buzzer instead of an LED — proof the `onBoolProperty()` API isn't LED-specific.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Buzzer. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Grove Buzzer into **D6**.
- **ESP32:** **GPIO 4** (selected automatically — GPIO 6 is a flash pin and is never usable).
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Property** named exactly **`buzzerOn`**, type **Boolean**, tick **Writable** → **Save**. Then **Views → Generate default views** → **Publish**. On the device page, flip **`buzzerOn`** and click **Save**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 3_Control_From_Cloud → 02_BuzzerCloudControl**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**Use it:** open the generated view → flip **`buzzerOn`** **On** → **Save**.
**You'll see:** `matched registered property 'buzzerOn' = true`, toggle **Pending → Accepted**, and a steady 1 kHz tone. Flip **Off** + **Save** → silent (`= false`).
**Try:** a buzzer is loud — set it **On** for a second then **Off**, confirming you can start/stop it purely from the cloud.

### Control From Cloud · 03 — Clap to Toggle LED (`03_ClapToToggleLed`)
**What it teaches:** A local event (a clap detected by the sound sensor) toggles the LED, and `reportBoolProperty()` pushes that change up so the dashboard toggle reflects claps too — two-way awareness, not just cloud-to-device.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Sound Sensor + Grove LED. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** Sound Sensor into **A2**, LED into **D7**.
- **ESP32:** sound on **GPIO 34** (ADC1), LED on **GPIO 2** (selected automatically — GPIO 7 is a flash pin). ESP32 ADC is 12-bit; re-tune `CLAP_THRESHOLD` (default 100).
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Property** named exactly **`ledState`**, **Boolean**, **Writable** → **Save**. Then **Views → Generate default views** → **Publish**. On the device page, flip **`ledState`** and click **Save**. Because this sketch also calls `reportBoolProperty()`, the property updates when the BOARD changes it (a clap), not only from the cloud.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 3_Control_From_Cloud → 03_ClapToToggleLed**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**Use it:** clap once near the sound sensor — the LED toggles. You can also flip **`ledState`** in the view + **Save**; a later clap toggles from that new state.
**You'll see:** on a cloud change, `matched registered property 'ledState' = true`; on a clap, the LED flips and the dashboard toggle updates on its own to match. A 500 ms cooldown stops an echo from double-toggling.
**Try:** clap a few times and watch the dashboard toggle track each clap without you touching it — that's `reportBoolProperty()`. If claps don't register (or every noise triggers), adjust `CLAP_THRESHOLD`.

### Control From Cloud · 04 — Auto Night Light (`04_AutoNightLight`)
**What it teaches:** An automatic sustained-condition rule (LED on when dark, off when bright) that you can override from the dashboard, with the local decision reported back to the cloud.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Light Sensor + Grove LED. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** Light Sensor into **A4**, LED into **D7**.
- **ESP32:** light on **GPIO 35** (ADC1), LED on **GPIO 2** (selected automatically — GPIO 7 is a flash pin). ESP32 ADC is 12-bit; re-tune `DARK_THRESHOLD_LOW` (280) and `DARK_THRESHOLD_HIGH` (320).
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Property** named exactly **`ledState`**, **Boolean**, **Writable** → **Save**. The sketch also calls `AzureIoT.publish("light", ...)` — add a **Telemetry** capability named **`light`**, type **Double**. Then **Views → Generate default views** → **Publish**. This sketch uses `reportBoolProperty()`, so the property updates when the light level crosses a threshold, not only from the cloud.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 3_Control_From_Cloud → 04_AutoNightLight**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**Use it:** cover the light sensor (or dim the room) → the LED turns on automatically; brighten it → LED off. To override, flip **`ledState`** in the view + **Save** (e.g. force it off in daylight).
**You'll see:** when the light level crosses a threshold the LED changes and the dashboard toggle updates on its own; on a cloud override, `matched registered property 'ledState' = ...`. The override lasts only until the next light-level crossing takes over.
**Try:** watch the **`light`** chart while covering/uncovering the sensor; note the two-threshold dead band (280/320) that stops flickering at the boundary.

### Control From Cloud · 05 — Comfort Alarm (`05_ComfortAlarm`)
**What it teaches:** A real-world automatic alarm — buzzer sounds when it's too hot/humid — that you can mute from the dashboard, showing a writable *control* property (`muted`) kept deliberately separate from a *read-only* telemetry report (`buzzerOn`).
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Temperature & Humidity Sensor (DHT11) + Grove Buzzer. **Extra library:** "Grove Temperature And Humidity Sensor" — install via Library Manager (Seeed).
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** DHT11 into **D2**, Buzzer into **D6**.
- **ESP32:** DHT11 on **GPIO 2**, buzzer on **GPIO 4** (selected automatically — GPIO 6 is a flash pin). Power the DHT11 from **3V3, not 5V**.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Property** named exactly **`muted`**, **Boolean**, **Writable** → **Save**. This sketch also publishes plain telemetry — add a **Telemetry** capability, type **Double**, for each of **`humidity`**, **`temperature`**, **`buzzerOn`**. Then **Views → Generate default views** → **Publish**. On the device page, flip **`muted`** + **Save**. Note: **`buzzerOn`** is telemetry here (a read-only report of whether the buzzer is sounding), NOT a writable property — that's why it has a different name from the `muted` control.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 3_Control_From_Cloud → 05_ComfortAlarm**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**Use it:** warm or breathe on the DHT11 until temperature/humidity cross the thresholds (default 30 °C / 70%) — the buzzer sounds. To silence it without unplugging, flip **`muted`** **On** in the view + **Save**.
**You'll see:** on the mute command, `matched registered property 'muted' = true`, the buzzer goes silent, and **`buzzerOn`** telemetry drops to `0` right away. The `temperature`/`humidity` charts update ~once a second.
**Try:** mute while it's buzzing, then unmute — the alarm re-sounds only if the reading is still above the high threshold, thanks to the dead band (30/28 °C, 70/65%).

### Control From Cloud · 06 — Button + LED Two-Way Sync (`06_ButtonLedTwoWaySync`)
**What it teaches:** True symmetric two-way control — the physical button and the dashboard toggle can each turn the LED on/off, and whichever acted most recently wins.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove Button + Grove LED. **Extra library:** None.
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** Button into **D4**, LED into **D7**.
- **ESP32:** button on **GPIO 4**, LED on **GPIO 2** (selected automatically — GPIO 7 is a flash pin).
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Property** named exactly **`ledState`**, **Boolean**, **Writable** → **Save**. The sketch also calls `AzureIoT.publish("button", ...)` — add a **Telemetry** capability named **`button`**, type **Double**. Then **Views → Generate default views** → **Publish**. On the device page, flip **`ledState`** + **Save**. Because the sketch also calls `reportBoolProperty()`, the property updates when the BOARD changes it (a button press), not only from the cloud.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 3_Control_From_Cloud → 06_ButtonLedTwoWaySync**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**Use it:** press the physical button — the LED toggles and the dashboard follows. Or flip **`ledState`** in the view + **Save** — the LED toggles from the cloud. Either side can act; the most recent action wins.
**You'll see:** on a button press, the LED flips and the dashboard toggle updates on its own; on a cloud change, `matched registered property 'ledState' = true`/`false`. The **`button`** telemetry reflects the raw reading.
**Try:** set the LED **On** from the dashboard, then press the button — it immediately toggles it **Off**, proving neither side permanently "owns" the LED.

### Control From Cloud · 07 — LED + LCD Cloud Control (`07_LedLcdCloudControl`)
**What it teaches:** The Mode-2 writable-property control from 01, plus a Grove LCD RGB Backlight that displays the cloud command so you can *see* it arrive — "LED: ON"/"LED: OFF" with a green/red backlight.
**Hardware:** Uno WiFi Rev2 or ESP32 + Grove LED + Grove LCD RGB Backlight (I2C). **Extra library:** "Grove - LCD RGB Backlight" v1.0.2+ — install via Library Manager (1.0.2+ auto-detects both v4.0 and v5.0 modules).
**Wire it:**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Grove LED into **D7**; plug the LCD into any I2C port.
- **ESP32:** LED on **GPIO 2** (selected automatically — GPIO 7 is a flash pin). The LCD is I2C — plug into any I2C port; no pin to set.
**Set up in IoT Central (one-time):** Go to **Device templates**, open your template, and click its **model** interface, then: **+ Add capability → Property** named exactly **`ledState`**, **Boolean**, **Writable** → **Save**. Then **Views → Generate default views** → **Publish**. On the device page, flip **`ledState`** + **Save**.
**In the Arduino IDE:**
1. **File → Examples → AzureIoT → 3_Control_From_Cloud → 07_LedLcdCloudControl**.
2. Fill in `config.h`; **Upload**; **Serial Monitor at 9600 baud**; wait for `MQTT connected.`
**Use it:** before the first cloud value arrives the LCD shows "Waiting for cloud...". Open the generated view → flip **`ledState`** **On** → **Save**.
**You'll see:** `matched registered property 'ledState' = true`; the LCD switches to "Cloud control: / LED: ON" with a **green** backlight and the LED lights. Flip **Off** + **Save** → "LED: OFF" with a **red** backlight (`= false`).
**Try:** flip back and forth a few times and watch the backlight snap green/red — the LCD makes each cloud command visible even if you can't see the LED.

---

## 4 · React To Another Device 🔁 — react to another board's data (`onRemoteTelemetry()`)

> These need **two boards** and an **API token** (not a device key). Board A sends
> `temperature`; the board you set up here (Board B) reads it and reacts.

### React To Another Device · 01 — Remote Temperature Alarm (`01_RemoteTemperatureAlarm`)
**What it teaches:** How to make one board watch a *different* board's `temperature` telemetry in Azure and sound a buzzer when it crosses 30 °C — all without this board ever sending data of its own.
**Hardware:** TWO boards — "Board A" (the sender, any board running a temperature example) and THIS board ("Board B", the reader) + a Grove Buzzer. **Extra library:** None.
**Wire it (Board B):**
- **Uno WiFi Rev2 (+ Grove shield):** plug the Grove Buzzer into port **D6**.
- **ESP32:** GPIO 4 (selected automatically — the sketch avoids GPIO 6, a flash pin).
**Set up (one-time):**
1. **Board A must already be publishing `temperature`** — e.g. flash a second board with **Send To Cloud → 09_TemperatureSensor**, set up as its OWN device (like Mode 1). Note **Board A's Device ID**. (In a class, Board A can be a classmate's board.)
2. **Create an API token:** IoT Central → **Permissions → API tokens → + New** → name it → role **App Operator** → **Generate** → **copy the whole `SharedAccessSignature ...` string** (you can't see it again).
3. **Find your app subdomain:** the `myapp` in `myapp.azureiotcentral.com`.
**In the Arduino IDE (Board B):**
1. **File → Examples → AzureIoT → 4_React_To_Another_Device → 01_RemoteTemperatureAlarm**.
2. Click the **`config.h`** tab; set `WIFI_SSID`/`WIFI_PASSWORD`, `IOTC_REMOTE_APP_SUBDOMAIN` (the subdomain), and `IOTC_REMOTE_API_TOKEN` (the full token). **Leave `IOTC_ID_SCOPE`/`IOTC_DEVICE_ID`/`IOTC_DEVICE_KEY` as placeholders** — this board is pull-only and doesn't use them.
3. In the `.ino`, set **`REMOTE_DEVICE_ID`** (near the top, default `"arduino1"`) to **Board A's Device ID**.
4. **Upload**, open **Serial Monitor at 9600 baud**.
**You'll see:** `Remote temperature: 24.50` printed every ~15 s (the polled value from Board A); when Board A's reading rises above 30 °C the buzzer sounds, and it goes quiet once the reading drops below 29 °C (a 1-degree dead band stops it chattering).
**Try:** warm Board A's temperature sensor with your fingers to push it past 30 °C and watch Board B react.
**Notes:** it **polls** (default every 15 s), so there's a lag; and Board B is **pull-only** — it never appears as sending its own data in IoT Central.

### React To Another Device · 02 — Remote Temperature on LCD (`02_RemoteTemperatureLcd`)
**What it teaches:** How to make one board pull a *different* board's `temperature` telemetry from Azure and show it live on a Grove LCD — again without this board ever sending data of its own.
**Hardware:** TWO boards — "Board A" (the sender) and THIS board ("Board B", the reader) + a Grove LCD RGB Backlight. **Extra library:** "Grove - LCD RGB Backlight" v1.0.2+ — install via Library Manager (1.0.2+ auto-detects both v4.0 and v5.0 modules).
**Wire it (Board B):**
- **Uno WiFi Rev2 (+ Grove shield):** the LCD is I2C — plug it into any I2C port; no pin to set.
- **ESP32:** the LCD is I2C — plug into any I2C port; no pin to set.
**Set up (one-time):**
1. **Board A must already be publishing `temperature`** — e.g. **Send To Cloud → 09_TemperatureSensor** as its OWN device. Note **Board A's Device ID**.
2. **Create an API token:** IoT Central → **Permissions → API tokens → + New** → role **App Operator** → **Generate** → copy the whole `SharedAccessSignature ...` string.
3. **Find your app subdomain** (the `myapp` in `myapp.azureiotcentral.com`).
**In the Arduino IDE (Board B):**
1. **File → Examples → AzureIoT → 4_React_To_Another_Device → 02_RemoteTemperatureLcd**.
2. Set `WIFI_SSID`/`WIFI_PASSWORD`, `IOTC_REMOTE_APP_SUBDOMAIN`, `IOTC_REMOTE_API_TOKEN` in `config.h` (leave the three identity values as placeholders).
3. In the `.ino`, set **`REMOTE_DEVICE_ID`** to **Board A's Device ID**.
4. **Upload**, open **Serial Monitor at 9600 baud**.
**You'll see:** the LCD first shows `Waiting for` / `remote data...`; then every ~15 s it refreshes to `Remote temp:` on the top row and the value plus ` C` below (e.g. `24.50 C`). The same value prints to Serial as `Remote temperature: 24.50`.
**Try:** warm Board A's sensor to push it past 30 °C and watch the number on Board B's LCD climb.
**Notes:** it **polls** (default every 15 s), so there's a lag; and Board B is **pull-only** — it never appears as sending its own data in IoT Central.

---

*Stuck on any step? See the [Tutorial's troubleshooting](TUTORIAL.md#part-e--when-something-doesnt-work) and the [README troubleshooting table](README.md#troubleshooting).*
