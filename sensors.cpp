#include "sensors.h"
#include "config.h"
#include <math.h>

// ---------------------------------------------------------------------
// Each function follows the same shape: read the pin, do the sensor's
// math, and return false instead of publishing a bogus number if
// something looks wrong (e.g. an ADC reading that would divide by zero).
// This is the pattern to copy when you add your own sensor.
// ---------------------------------------------------------------------

#if ENABLE_SENSOR_TEMPERATURE
static bool read_temperature(float *out) {
    // Same formula as Module 2/4's Grove Temperature Sensor (thermistor).
    const int B = 4275;
    const long R0 = 100000;
    int a = analogRead(PIN_TEMPERATURE);
    if (a <= 0) return false; // avoid divide-by-zero if the sensor is unplugged
    float R = 1023.0f / (float)a - 1.0f;
    R = (float)R0 * R;
    *out = 1.0f / (log(R / (float)R0) / B + 1.0f / 298.15f) - 273.15f;
    return true;
}
#endif

#if ENABLE_SENSOR_ROTARY_ANGLE
static bool read_rotary_angle(float *out) {
    // Same formula as Module 2/4's Grove Rotary Angle Sensor.
    const float ADC_REF = 5.0f;
    const float FULL_ANGLE = 300.0f;
    const float GROVE_VCC = 5.0f;
    int sensorValue = analogRead(PIN_ROTARY_ANGLE);
    float voltage = (float)sensorValue * ADC_REF / 1023.0f;
    *out = (voltage * FULL_ANGLE) / GROVE_VCC;
    return true;
}
#endif

#if ENABLE_SENSOR_SOUND
static bool read_sound(float *out) {
    // Same 32-sample averaging as Module 2/4's Grove Sound Sensor.
    long sum = 0;
    for (int i = 0; i < 32; i++) {
        sum += analogRead(PIN_SOUND);
    }
    sum >>= 5;
    *out = (float)sum;
    return true;
}
#endif

#if ENABLE_SENSOR_TOUCH
static bool read_touch(float *out) {
    *out = (float)digitalRead(PIN_TOUCH);
    return true;
}
#endif

#if ENABLE_SENSOR_TURBIDITY
static bool read_turbidity(float *out) {
    // Same formula as Module 2/4's Grove Turbidity Sensor.
    int sensorValue = analogRead(PIN_TURBIDITY);
    *out = sensorValue * (5.0f / 1024.0f);
    return true;
}
#endif

#if ENABLE_SENSOR_HUMIDITY
// Requires the Seeed "Grove Temperature And Humidity Sensor" library, added
// exactly as in Module 2's Tutorial 9: Sketch > Include Library > Add .ZIP
// Library, using the DHTLib.zip from
// https://github.com/Seeed-Studio/Grove_Temperature_And_Humidity_Sensor
#include <DHT.h>
static DHT s_dht(PIN_HUMIDITY, DHT11);
static bool s_dhtBegun = false;
static bool read_humidity(float *out) {
    if (!s_dhtBegun) { s_dht.begin(); s_dhtBegun = true; }
    float temp_hum_val[2] = {0};
    if (s_dht.readTempAndHumidity(temp_hum_val) != 0) return false;
    *out = temp_hum_val[0]; // humidity %RH; swap to [1] if you want temperature instead
    return true;
}
#endif

// ---------------------------------------------------------------------
// The table. This is the ONLY place you list which sensors are active --
// uno_wifi_azure.ino reads this table generically and never mentions any
// sensor by name.
// ---------------------------------------------------------------------
const SensorDef g_sensorTable[] = {
#if ENABLE_SENSOR_TEMPERATURE
    { "temperature", read_temperature },
#endif
#if ENABLE_SENSOR_ROTARY_ANGLE
    { "rotaryAngle", read_rotary_angle },
#endif
#if ENABLE_SENSOR_SOUND
    { "sound", read_sound },
#endif
#if ENABLE_SENSOR_TOUCH
    { "touch", read_touch },
#endif
#if ENABLE_SENSOR_TURBIDITY
    { "turbidity", read_turbidity },
#endif
#if ENABLE_SENSOR_HUMIDITY
    { "humidity", read_humidity },
#endif
};
const size_t g_sensorCount = sizeof(g_sensorTable) / sizeof(g_sensorTable[0]);
