#include <stdio.h>
#include <string.h>
#include <math.h>

static int formatFloat2dp(char *buf, size_t bufCap, float value) {
    if (isnan(value) || isinf(value)) {
        return snprintf(buf, bufCap, "0.00");
    }
    int negative = value < 0.0f;
    float absValue = negative ? -value : value;
    long scaled = (long)(absValue * 100.0f + 0.5f);
    if (scaled == 0) negative = 0;
    long intPart = scaled / 100;
    long fracPart = scaled % 100;
    return snprintf(buf, bufCap, "%s%ld.%02ld", negative ? "-" : "", intPart, fracPart);
}

static void appendJsonEscaped(char *buf, size_t bufCap, size_t *pos, const char *s) {
    while (*s != '\0' && *pos + 1 < bufCap) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            if (*pos + 2 >= bufCap) break;
            buf[(*pos)++] = '\\';
            buf[(*pos)++] = (char)c;
        } else if (c == '\n' && *pos + 2 < bufCap) {
            buf[(*pos)++] = '\\'; buf[(*pos)++] = 'n';
        } else if (c == '\r' && *pos + 2 < bufCap) {
            buf[(*pos)++] = '\\'; buf[(*pos)++] = 'r';
        } else if (c == '\t' && *pos + 2 < bufCap) {
            buf[(*pos)++] = '\\'; buf[(*pos)++] = 't';
        } else if (c < 0x20) {
            if (*pos + 6 >= bufCap) break;
            int n = snprintf(buf + *pos, bufCap - *pos, "\\u%04x", c);
            if (n > 0) *pos += (size_t)n;
        } else {
            buf[(*pos)++] = (char)c;
        }
        s++;
    }
}

int main() {
    char buf[32];
    float cases[] = {0.0f, -0.0f, 1.005f, -1.005f, 0.005f, 99.995f, -99.995f,
                      123456.789f, -0.001f, 0.0001f, 3.14159f};
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        formatFloat2dp(buf, sizeof(buf), cases[i]);
        printf("formatFloat2dp(%g) = %s\n", cases[i], buf);
    }
    formatFloat2dp(buf, sizeof(buf), NAN);
    printf("formatFloat2dp(NAN) = %s\n", buf);
    formatFloat2dp(buf, sizeof(buf), INFINITY);
    printf("formatFloat2dp(INF) = %s\n", buf);
    formatFloat2dp(buf, sizeof(buf), -INFINITY);
    printf("formatFloat2dp(-INF) = %s\n", buf);

    // tiny output buffer -- does formatFloat2dp ever write past bufCap? snprintf is safe by contract.
    char tiny[4];
    int n = formatFloat2dp(tiny, sizeof(tiny), 12345.678f);
    printf("formatFloat2dp truncated (bufCap=4): wrote-would-be=%d buf='%s'\n", n, tiny);

    // appendJsonEscaped: control chars, quotes, backslash, near-full buffer
    char jbuf[16];
    size_t pos = 0;
    appendJsonEscaped(jbuf, sizeof(jbuf), &pos, "ab\"c\\d\ne");
    jbuf[pos] = 0;
    printf("appendJsonEscaped -> '%s' (pos=%zu)\n", jbuf, pos);

    // near-boundary: exactly fills buffer with an escape sequence
    char jbuf2[5];
    size_t pos2 = 0;
    appendJsonEscaped(jbuf2, sizeof(jbuf2), &pos2, "\"\"\"\"\"\"\""); // all need escaping
    jbuf2[pos2] = 0;
    printf("appendJsonEscaped tight buffer -> '%s' (pos=%zu, cap=5)\n", jbuf2, pos2);

    return 0;
}
