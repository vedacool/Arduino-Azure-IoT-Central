// Example 08 -- Buzzer Off (Command)
//
// Demonstrates COMMANDS (cloud-triggered actions) instead of writable
// properties. Two no-argument commands control a Grove Buzzer from the IoT
// Central dashboard:
//   "buzzerOn"  -- start a tone
//   "buzzerOff" -- stop it
//
// Why a command and not a property? A command is IMPERATIVE: it fires EVERY
// time you press its button, no matter the current state. A writable property
// only notifies the board when its value CHANGES -- so if the property were
// already "off" but the buzzer somehow got left on, setting "off" again would
// send nothing and you could not silence it. As a command, "buzzerOff" ALWAYS
// silences the buzzer. This is the Tier-1 (no-parameter) form of onCommand().
//
// IoT Central setup: Device templates > your template > click the model
// (the row tagged Root) >
//   + Add capability -> Capability type COMMAND, Name "buzzerOn"
//   + Add capability -> Capability type COMMAND, Name "buzzerOff"
// Save -> Publish. Each appears as a BUTTON on the device page (no view to
// generate -- commands are buttons, not toggles).
//
// Setup: edit config.h in this folder with your Wi-Fi + Azure credentials
// before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_BUZZER = 4;   // ESP32: safe GPIO. GPIO 6 is a flash pin on ESP32 -- never usable
#else
const int PIN_BUZZER = 6;   // Uno WiFi Rev2 + Grove Base Shield: Grove D6
#endif

void buzzerOn() {
    tone(PIN_BUZZER, 1000);
    Serial.println("Command: buzzer ON");
}

void buzzerOff() {
    noTone(PIN_BUZZER);        // always silences, whatever the current state
    Serial.println("Command: buzzer OFF");
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_BUZZER, OUTPUT);

    // Register commands BEFORE begin(). These are the no-argument (Tier 1)
    // form -- the library auto-replies 200 after each handler returns.
    AzureIoT.onCommand("buzzerOn", buzzerOn);
    AzureIoT.onCommand("buzzerOff", buzzerOff);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // handles reconnects AND delivering commands to the handlers above
}
