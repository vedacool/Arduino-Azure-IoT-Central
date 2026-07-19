#include "dps_client.h"
#include "sas_token.h"
#include <string.h>
#include <stdlib.h>

static const char DPS_HOST[] = "global.azure-devices-provisioning.net";
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

// Sends one HTTPS request over an already-connected WiFiSSLClient and reads
// the response body into `body` (headers are skipped). Returns HTTP status
// code, or -1 on failure/timeout.
static int httpRequest(WiFiSSLClient &client, const char *method, const char *path,
                        const char *authHeader, const char *body, size_t bodyLen,
                        char *respBody, size_t respBodyCap) {
    if (!client.connect(DPS_HOST, 443)) return -1;

    client.print(method);
    client.print(" ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(DPS_HOST);
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
    String statusLine = client.readStringUntil('\n');
    int statusCode = -1;
    int firstSpace = statusLine.indexOf(' ');
    if (firstSpace > 0) statusCode = statusLine.substring(firstSpace + 1).toInt();

    // Skip headers until blank line.
    while (client.connected() || client.available()) {
        String line = client.readStringUntil('\n');
        if (line.length() <= 1) break; // "\r\n" only
    }

    // Read remaining body (chunked or not -- DPS responses are small and not
    // chunk-encoded in practice for this call, so a straight read is fine).
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
                   DpsResult &out) {
    // resourceUri for DPS = "{idScope}/registrations/{deviceId}"
    char resourceUri[160];
    snprintf(resourceUri, sizeof(resourceUri), "%s/registrations/%s", idScope, deviceId);

    unsigned long expiry = WiFi.getTime() + 3600UL;
    char sasToken[300];
    if (build_sas_token(deviceKeyB64, resourceUri, expiry, sasToken, sizeof(sasToken)) == 0) {
        return false;
    }

    char path[200];
    snprintf(path, sizeof(path), "/%s/registrations/%s/register?api-version=%s",
             idScope, deviceId, DPS_API_VERSION);

    char reqBody[80];
    int bodyLen = snprintf(reqBody, sizeof(reqBody), "{\"registrationId\":\"%s\"}", deviceId);

    char respBody[512];
    WiFiSSLClient client;
    int status = httpRequest(client, "PUT", path, sasToken, reqBody, (size_t)bodyLen,
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

        WiFiSSLClient pollClient;
        int pollStatus = httpRequest(pollClient, "GET", pollPath, sasToken, nullptr, 0,
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
