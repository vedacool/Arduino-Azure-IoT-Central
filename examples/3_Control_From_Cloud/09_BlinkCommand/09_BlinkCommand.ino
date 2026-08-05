// Example 09 -- Blink (Command WITH a value)
//
// Demonstrates a COMMAND that carries a value -- the Tier-2 form of
// onCommand(). The dashboard command "blink" takes a number, and the board
// blinks a Grove LED that many times. A command is the right tool for a
// one-shot action like this; a writable property models a persistent state,
// not "do it N times right now".
//
// IoT Central setup: Device templates > your template > click the model
// (the row tagged Root) >
//   + Add capability -> Capability type COMMAND, Name "blink"
//   click the v chevron at the end of the row to expand it
//   turn Request ON -> Request Schema INTEGER (leave Response off)
// Save -> Publish. On the device page the command shows a NUMBER box + a Run
// button. Because the request is a PRIMITIVE Integer, IoT Central sends the
// bare value (e.g. 3), which atoi() reads directly. (An Object request would
// arrive as JSON instead -- keep it a primitive for this example.)
//
// Setup: edit config.h in this folder with your Wi-Fi + Azure credentials
// before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_LED = 2;   // ESP32: onboard LED. GPIO 7 is a flash pin on ESP32 -- never usable
#else
const int PIN_LED = 7;   // Uno WiFi Rev2 + Grove Base Shield: Grove D7
#endif

// request is the bare value IoT Central sent, e.g. "3". atoi() turns it into
// a number; if the command carried no value, request is "" and atoi() is 0.
void blink(const char *request) {
    int count = atoi(request);
    if (count < 1)  count = 1;    // at least once
    if (count > 10) count = 10;   // cap: don't block loop() (and MQTT) too long
    Serial.print("Command: blink x");
    Serial.println(count);
    for (int i = 0; i < count; i++) {
        digitalWrite(PIN_LED, HIGH);
        delay(200);
        digitalWrite(PIN_LED, LOW);
        delay(200);
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(PIN_LED, OUTPUT);

    // Register BEFORE begin(). This is the with-value (Tier 2) form -- the
    // handler receives the request payload; the library still auto-replies 200.
    AzureIoT.onCommand("blink", blink);

    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop();
}
