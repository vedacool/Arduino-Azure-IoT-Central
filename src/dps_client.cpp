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
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
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

    char path[200];
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

    char operationId[80] = {0};
    if (!extractJsonString(respBody, "operationId", operationId, sizeof(operationId))) {
        return false;
    }

    // Poll operation status until "assigned" (or give up after ~10 tries).
    for (int attempt = 0; attempt < 10; attempt++) {
        delay(2000); // DPS typically asks for ~2s between polls (Retry-After)

        char pollPath[260];
        snprintf(pollPath, sizeof(pollPath),
                 "/%s/registrations/%s/operations/%s?api-version=%s",
                 idScope, deviceId, operationId, DPS_API_VERSION);

        SecureWiFiClient pollClient;
        int pollStatus = azureiot_http_request(pollClient, dpsGlobalHost, "GET", pollPath, sasToken, nullptr, 0,
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
