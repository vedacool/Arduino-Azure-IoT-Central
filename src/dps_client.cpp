#include "dps_client.h"
#include "sas_token.h"
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

// Reads one line (up to and including '\n', which is stripped, as is a
// trailing '\r') into buf. Returns line length, or -1 on timeout/overflow.
// No String, no heap allocation -- matches the fixed-buffer approach used
// everywhere else in this sketch.
static int readLine(SecureWiFiClient &client, char *buf, size_t bufCap, unsigned long timeoutMs) {
    size_t n = 0;
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        if (client.available()) {
            char c = (char)client.read();
            if (c == '\n') {
                if (n > 0 && buf[n-1] == '\r') n--;
                buf[n] = '\0';
                return (int)n;
            }
            if (n + 1 >= bufCap) {
                buf[n] = '\0';
                return (int)n; // line longer than our buffer; caller gets what fit
            }
            buf[n++] = c;
        } else if (!client.connected()) {
            break;
        } else {
            delay(5);
        }
    }
    buf[n] = '\0';
    return n > 0 ? (int)n : -1;
}

// Sends one HTTPS request over an already-connected SecureWiFiClient and reads
// the response body into `body` (headers are skipped). Returns HTTP status
// code, or -1 on failure/timeout.
static int httpRequest(SecureWiFiClient &client, const char *dpsGlobalHost,
                        const char *method, const char *path,
                        const char *authHeader, const char *body, size_t bodyLen,
                        char *respBody, size_t respBodyCap) {
    configureSecureClient(client);
    if (!client.connect(dpsGlobalHost, 443)) return -1;

    client.print(method);
    client.print(" ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(dpsGlobalHost);
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.print("Authorization: ");
    client.println(authHeader);
    if (bodyLen > 0) {
        client.print("Content-Length: ");
        client.println((unsigned long)bodyLen);
        client.println();
        client.write((const uint8_t *)body, bodyLen);
    } else {
        client.println();
    }

    unsigned long start = millis();
    while (!client.available() && millis() - start < 10000UL) {
        delay(20);
    }
    if (!client.available()) {
        client.stop();
        return -1;
    }

    // Status line, e.g. "HTTP/1.1 202 Accepted"
    char lineBuf[96];
    if (readLine(client, lineBuf, sizeof(lineBuf), 5000UL) < 0) {
        client.stop();
        return -1;
    }
    int statusCode = -1;
    char *sp = strchr(lineBuf, ' ');
    if (sp) statusCode = atoi(sp + 1);

    // Skip headers until the blank line.
    while (true) {
        int len = readLine(client, lineBuf, sizeof(lineBuf), 5000UL);
        if (len <= 0) break; // blank line (end of headers) or timeout/EOF
    }

    // Read remaining body (chunked or not -- DPS responses observed in
    // practice for this call are small and not chunk-encoded, so a straight
    // read is fine; if Azure ever changes that, extractJsonString() below
    // will simply fail to find its fields and dpsProvision() returns false
    // rather than misbehaving silently).
    size_t n = 0;
    unsigned long bodyStart = millis();
    while ((client.connected() || client.available()) && n < respBodyCap - 1 &&
           millis() - bodyStart < 8000UL) {
        if (client.available()) {
            respBody[n++] = (char)client.read();
        } else {
            delay(10);
        }
    }
    respBody[n] = '\0';
    client.stop();
    return statusCode;
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
    int status = httpRequest(client, dpsGlobalHost, "PUT", path, sasToken, reqBody, (size_t)bodyLen,
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
        int pollStatus = httpRequest(pollClient, dpsGlobalHost, "GET", pollPath, sasToken, nullptr, 0,
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
