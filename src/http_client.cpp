#include "http_client.h"
#include <string.h>
#include <stdlib.h>

// Reads one line (up to and including '\n', which is stripped, as is a
// trailing '\r') into buf. Returns line length, or -1 on timeout/overflow.
// No String, no heap allocation -- matches the fixed-buffer approach used
// everywhere else in this library.
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
            platformWatchdogPet(); // safe no-op if the watchdog was never enabled -- see enableWatchdog() in AzureIoT.h
            delay(5);
        }
    }
    buf[n] = '\0';
    return n > 0 ? (int)n : -1;
}

int azureiot_http_request(SecureWiFiClient &client, const char *host,
                           const char *method, const char *path,
                           const char *authHeader, const char *body, size_t bodyLen,
                           char *respBody, size_t respBodyCap) {
    configureSecureClient(client);
    if (!client.connect(host, 443)) return -1;

    client.print(method);
    client.print(" ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(host);
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
        platformWatchdogPet();
        delay(20);
    }
    if (!client.available()) {
        client.stop();
        return -1;
    }

    // Status line, e.g. "HTTP/1.1 200 OK"
    char lineBuf[96];
    if (readLine(client, lineBuf, sizeof(lineBuf), 5000UL) < 0) {
        client.stop();
        return -1;
    }
    int statusCode = -1;
    char *sp = strchr(lineBuf, ' ');
    if (sp) statusCode = atoi(sp + 1);

    // Skip headers until the blank line, noting Transfer-Encoding: chunked if
    // the server sends it. This client does NOT de-chunk; Azure DPS/IoT
    // Central responses observed in practice are small and not chunk-encoded.
    // If that ever changes, a raw read leaves chunk-size markers interleaved
    // in the body and the caller's JSON extraction silently returns
    // "not found" -- so surface it as an explicit diagnostic rather than a
    // mystery failure.
    bool chunked = false;
    while (true) {
        int len = readLine(client, lineBuf, sizeof(lineBuf), 5000UL);
        if (len <= 0) break; // blank line (end of headers) or timeout/EOF
        if (strstr(lineBuf, "Transfer-Encoding:") && strstr(lineBuf, "chunked")) {
            chunked = true;
        }
    }
    if (chunked) {
        Serial.println("AzureIoT: WARNING -- server sent a CHUNKED HTTP response, which this lightweight client does not de-chunk. The body will contain chunk-size markers and JSON parsing will likely fail. Please report this (endpoint + firmware version) so de-chunking can be added.");
    }

    // Read remaining body (see the chunked note above).
    size_t n = 0;
    unsigned long bodyStart = millis();
    while ((client.connected() || client.available()) && n < respBodyCap - 1 &&
           millis() - bodyStart < 8000UL) {
        if (client.available()) {
            respBody[n++] = (char)client.read();
        } else {
            platformWatchdogPet();
            delay(10);
        }
    }
    respBody[n] = '\0';
    client.stop();
    return statusCode;
}
