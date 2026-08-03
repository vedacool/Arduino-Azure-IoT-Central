// Example 22 -- Ultrasonic Distance (Grove Ultrasonic Ranger)
//
// Grove Ultrasonic Ranger on digital pin 7, publishing the measured
// distance (in centimetres) to Azure IoT Central. The Grove Ultrasonic
// Ranger uses a SINGLE signal pin for both the trigger pulse and the echo
// -- the small helper below drives that protocol directly, so no extra
// library needs installing.
//
// Setup: edit config.h in this folder with your
// Wi-Fi + Azure credentials before uploading.

#include <AzureIoT.h>
#include "config.h"

#if defined(ARDUINO_ARCH_ESP32)
const int PIN_ULTRASONIC = 18;  // ESP32: a regular GPIO -- the SIG pin is driven as both output and input
#else
const int PIN_ULTRASONIC = 7;   // Uno WiFi Rev2 + Grove Base Shield: Grove D7
#endif

// Reads the Grove Ultrasonic Ranger's single SIG pin: send a short trigger
// pulse, then time how long the echo stays HIGH. Distance in cm is
// echo_us / 29 / 2 (sound travels ~29 us per cm, and the echo covers the
// round trip, so divide by 2). The timeout is bounded to ~30 ms -- well past
// the sensor's ~400 cm / ~23 ms maximum -- so a missing echo can't stall
// loop(), which must keep running for reconnects and the watchdog. Returns 0
// when nothing is in range within the timeout.
long readUltrasonicCm(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delayMicroseconds(2);
    digitalWrite(pin, HIGH);
    delayMicroseconds(5);
    digitalWrite(pin, LOW);
    pinMode(pin, INPUT);
    long echo_us = pulseIn(pin, HIGH, 30000L);
    return echo_us / 29 / 2;
}

void setup() {
    Serial.begin(115200);
    AzureIoT.begin(WIFI_SSID, WIFI_PASSWORD, IOTC_ID_SCOPE, IOTC_DEVICE_ID, IOTC_DEVICE_KEY);
}

void loop() {
    AzureIoT.loop(); // always call this once per loop() -- handles reconnects + sending

    long distanceCm = readUltrasonicCm(PIN_ULTRASONIC);
    AzureIoT.publish("distance", (float)distanceCm);

    delay(250); // give the sensor a gap between pings; loop() decides when to actually send
}
