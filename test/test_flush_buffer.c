#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

static int formatFloat2dp(char *buf, size_t bufCap, float value) {
    if (isnan(value) || isinf(value)) return snprintf(buf, bufCap, "0.00");
    int negative = value < 0.0f;
    float absValue = negative ? -value : value;
    long scaled = (long)(absValue * 100.0f + 0.5f);
    long intPart = scaled / 100;
    long fracPart = scaled % 100;
    return snprintf(buf, bufCap, "%s%ld.%02ld", negative ? "-" : "", intPart, fracPart);
}

static int appendField(char *buf, size_t bufCap, size_t *pos, int first,
                        const char *key, float value) {
    char valueStr[24];
    int vn = formatFloat2dp(valueStr, sizeof(valueStr), value);
    if (vn <= 0 || (size_t)vn >= sizeof(valueStr)) return 0;

    int n = snprintf(buf + *pos, bufCap - *pos, "%s\"%s\":%s",
                      first ? "" : ",", key, valueStr);
    if (n < 0 || (size_t)n >= bufCap - *pos) return 0;
    *pos += (size_t)n;
    return 1;
}

int main() {
    // Simulate flush() with 16 staged slots, each with a very long key
    // (bigger than anything the shipped examples use, but a user could pass
    // any string literal as a key).
    char payload[220];
    const size_t cap = sizeof(payload);
    size_t pos = 0;
    if (pos < cap - 1) payload[pos++] = '{';
    int first = 1;

    char longkey[64];
    for (int i = 0; i < 16; i++) {
        snprintf(longkey, sizeof(longkey), "some_quite_long_sensor_key_name_number_%02d", i);
        int ok = appendField(payload, sizeof(payload), &pos, first, longkey, 12345.6789f + i);
        printf("slot %d key='%s' ok=%d pos=%zu\n", i, longkey, ok, pos);
        if (ok) first = 0;
        else break; // matches real flush(): "ran out of room -- publish what fit"
    }
    if (pos < cap - 1) payload[pos++] = '}';
    payload[pos < cap ? pos : cap - 1] = '\0';
    printf("Final payload (len=%zu, cap=%zu): %s\n", pos, cap, payload);
    printf("strlen(payload)=%zu (must be <= cap-1=%zu)\n", strlen(payload), cap-1);
    if (strlen(payload) > cap - 1) { printf("OVERFLOW BUG\n"); return 1; }
    return 0;
}
