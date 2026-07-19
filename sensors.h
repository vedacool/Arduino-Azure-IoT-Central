#ifndef SENSORS_H
#define SENSORS_H
#include <Arduino.h>

// Adding a new sensor to this sketch means exactly two edits, both in
// sensors.cpp: (1) write a `bool read_xxx(float *out)` function, and
// (2) add one line to g_sensorTable[] at the bottom of this file's .cpp.
// Nothing in uno_wifi_azure.ino, dps_client.cpp, or the MQTT/SAS-token code
// needs to change -- the payload builder in the .ino iterates this table
// generically.
//
// A read function returns false if the reading should be treated as
// invalid/unavailable this cycle (e.g. a thermistor divide-by-zero, or a
// sensor whose warm-up time hasn't elapsed yet) -- publishTelemetry() will
// just skip that key for this publish rather than sending garbage.

typedef bool (*SensorReadFn)(float *out);

struct SensorDef {
    const char *jsonKey; // key name in the published JSON, e.g. "temperature"
    SensorReadFn read;
};

// Defined in sensors.cpp. Enable/disable individual sensors from config.h
// (see the ENABLE_SENSOR_* defines there) -- disabled ones are compiled out
// entirely, so they cost zero flash/RAM if you don't use them.
extern const SensorDef g_sensorTable[];
extern const size_t g_sensorCount;

#endif
