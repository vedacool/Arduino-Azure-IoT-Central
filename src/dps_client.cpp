#include "dps_client.h"
#include "sas_token.h"
#include "http_client.h"
#include <string.h>
#include <stdlib.h>

static const char DPS_API_VERSION[] = "2019-03-31";

// Very small helper: find `"key":"value"` in a JSON blob and copy value into out.
// Not a general JSON parser -- DPS responses are simple enough that substring
// extraction is reliable and costs far less flash/RAM than pulling in
// ArduinoJson for three fields.
static bool extractJsonString(const char *json, const char *key, char *out, size_t outCap) {
    char pattern[40];
    int pn = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if (pn < 0 || (size_t)pn >= sizeof(pattern)) return false; // key too long to match safely -- don't strstr a truncated pattern
    const char *p = strstr(json, pattern);
    if (!p) return false;
    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p) return false;
    p++;
    while (*p == ' ') p++;
    if (*p != '"') return false;
    p++;
    const char *end = strchr(p, '"');
    if (!end) return false;
    size_t len = (size_t)(end - p);
    if (len >= outCap) len = outCap - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return true;
}

bool dpsProvision(const char *idScope, const char *deviceId, const char *deviceKeyB64,
                   const char *dpsGlobalHost, const char *modelId, DpsResult &out) {
    // resourceUri for DPS = "{idScope}/registrations/{deviceId}"
    char resourceUri[160];
    snprintf(resourceUri, sizeof(resourceUri), "%s/registrations/%s", idScope, deviceId);

    unsigned long expiry = platformGetUnixTime() + 3600UL;
    char sasToken[300];
    if (build_sas_token(deviceKeyB64, resourceUri, expiry, sasToken, sizeof(sasToken)) == 0) {
        return false;
    }

    // 260 (not 200): this buffer is reused for the longer poll path in the
    // loop below, once the register path is no longer needed -- avoids a
    // second 260-byte stack buffer on the tight 6KB megaAVR provisioning frame.
    char path[260];
    snprintf(path, sizeof(path), "/%s/registrations/%s/register?api-version=%s",
             idScope, deviceId, DPS_API_VERSION);

    // 260, not 80: with a modelId, the body is
    // {"registrationId":"...","payload":{"modelId":"..."}} -- the fixed
    // wrapper text alone is ~45 chars, plus deviceId (up to 64) and modelId
    // (dtmi strings can run long with nested namespaces -- sized to match
    // AzureIoT.cpp's s_modelId[128]).
    char reqBody[260];
    int bodyLen;
    if (modelId && modelId[0] != '\0') {
        bodyLen = snprintf(reqBody, sizeof(reqBody),
                            "{\"registrationId\":\"%s\",\"payload\":{\"modelId\":\"%s\"}}",
                            deviceId, modelId);
    } else {
        bodyLen = snprintf(reqBody, sizeof(reqBody), "{\"registrationId\":\"%s\"}", deviceId);
    }
    if (bodyLen <= 0 || (size_t)bodyLen >= sizeof(reqBody)) {
        return false; // deviceId/modelId combination too long to fit -- fail loudly rather than send a truncated (invalid) registration body
    }

    char respBody[512];
    SecureWiFiClient client;
    int status = azureiot_http_request(client, dpsGlobalHost, "PUT", path, sasToken, reqBody, (size_t)bodyLen,
                              respBody, sizeof(respBody));
    if (status != 202 && status != 200) {
        return false;
    }

    char operationId[128] = {0}; // 128 (not 80): generous headroom so a longer-than-expected operationId isn't silently truncated (which would make every poll query a non-existent operation)
    if (!extractJsonString(respBody, "operationId", operationId, sizeof(operationId))) {
        return false;
    }

    // Poll operation status until "assigned". ~30 tries x 2s = ~60s budget
    // (was 10 / ~20s): first-time registration or a busy DPS endpoint can take
    // longer than 20s to assign, and giving up early just makes begin() restart
    // provisioning from scratch. (This library doesn't parse the Retry-After
    // header -- http_client discards headers -- so the interval is a fixed 2s.)
    for (int attempt = 0; attempt < 30; attempt++) {
        delay(2000); // DPS typically asks for ~2s between polls (Retry-After)

        // Reuse `path` (the register path isn't needed once the PUT above
        // returned) rather than a second 260-byte stack buffer here.
        snprintf(path, sizeof(path),
                 "/%s/registrations/%s/operations/%s?api-version=%s",
                 idScope, deviceId, operationId, DPS_API_VERSION);

        SecureWiFiClient pollClient;
        int pollStatus = azureiot_http_request(pollClient, dpsGlobalHost, "GET", path, sasToken, nullptr, 0,
                                      respBody, sizeof(respBody));
        if (pollStatus != 200 && pollStatus != 202) continue;

        char status_str[24] = {0};
        extractJsonString(respBody, "status", status_str, sizeof(status_str));

        if (strcmp(status_str, "assigned") == 0) {
            bool gotHub = extractJsonString(respBody, "assignedHub", out.assignedHub, sizeof(out.assignedHub));
            bool gotDev = extractJsonString(respBody, "deviceId", out.deviceId, sizeof(out.deviceId));
            return gotHub && gotDev;
        }
        // status == "assigning" -> keep polling
    }
    return false;
}
