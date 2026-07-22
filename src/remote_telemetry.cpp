#include "remote_telemetry.h"
#include "http_client.h"
#include "platform.h"
#include <string.h>
#include <stdlib.h>

// Finds `"key":<number>` (unquoted, as JSON numbers are) and parses it as a
// float. Same substring-search philosophy as extractJsonBool()/
// extractJsonNumber() in AzureIoT.cpp -- not a general JSON parser, but
// reliable for the simple {"timestamp":"...","value":32.5} shape this
// specific IoT Central REST endpoint returns (confirmed against Microsoft's
// own documentation, not assumed).
static bool extractJsonFloat(const char *json, const char *key, float *out) {
    char pattern[24];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return false;
    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p) return false;
    p++;
    while (*p == ' ') p++;
    char *endptr;
    // strtod(), not strtof() -- confirmed via a real compile error on actual
    // Uno WiFi Rev2 hardware that this board's avr-libc doesn't declare
    // strtof() at all (some avr-libc versions lack it entirely, or gate it
    // behind a feature-test macro Arduino's build doesn't set), even though
    // it's a fairly standard C function on desktop platforms. strtod() (and
    // atof()) are confirmed present across avr-libc versions, so this
    // parses as double and casts down to float rather than risk the same
    // gap on strtof().
    double v = strtod(p, &endptr);
    if (endptr == p) return false;
    *out = (float)v;
    return true;
}

bool azureiot_poll_remote_telemetry(const char *appSubdomain, const char *apiToken,
                                     const char *remoteDeviceId, const char *telemetryName,
                                     float *outValue, int *outStatusCode) {
    // 128, not a tighter size: appSubdomain is user-supplied and this is
    // cheap to size generously -- checked below rather than assumed safe.
    char host[128];
    int hostLen = snprintf(host, sizeof(host), "%s.azureiotcentral.com", appSubdomain);
    if (hostLen <= 0 || (size_t)hostLen >= sizeof(host)) {
        if (outStatusCode) *outStatusCode = -1;
        return false;
    }

    // 300: "/api/devices/" (13) + a device ID (Azure's own cap is 128) +
    // "/telemetry/" (11) + a telemetry name (generous headroom) +
    // "?api-version=2022-07-31" (24) + NUL -- real margin, checked below.
    char path[300];
    int pathLen = snprintf(path, sizeof(path), "/api/devices/%s/telemetry/%s?api-version=2022-07-31",
                            remoteDeviceId, telemetryName);
    if (pathLen <= 0 || (size_t)pathLen >= sizeof(path)) {
        if (outStatusCode) *outStatusCode = -1;
        return false;
    }

    // The response is always small -- a timestamp string plus one number --
    // 256 is generous, not tight.
    char respBody[256];
    SecureWiFiClient client;
    int status = azureiot_http_request(client, host, "GET", path, apiToken, nullptr, 0,
                                        respBody, sizeof(respBody));
    if (outStatusCode) *outStatusCode = status;
    if (status != 200) return false;

    return extractJsonFloat(respBody, "value", outValue);
}
