#ifndef AZURE_IOT_HTTP_CLIENT_H
#define AZURE_IOT_HTTP_CLIENT_H
#include "platform.h"
#include <stddef.h>

// Sends one HTTPS request over a fresh SecureWiFiClient connection and reads
// the response body into `respBody` (headers are skipped). Returns the HTTP
// status code, or -1 on failure/timeout. `authHeader` is sent verbatim as
// the value of the Authorization header -- callers are responsible for its
// exact shape (e.g. a DPS SAS token, or an IoT Central "SharedAccessSignature
// sr=..." API token -- neither gets a "Bearer " prefix added here).
//
// Shared by dps_client.cpp (DPS provisioning) and the remote-telemetry
// polling feature (reading another device's telemetry via the IoT Central
// REST API) -- extracted into its own file rather than duplicated, since
// both need the exact same request/response handling.
int azureiot_http_request(SecureWiFiClient &client, const char *host,
                           const char *method, const char *path,
                           const char *authHeader, const char *body, size_t bodyLen,
                           char *respBody, size_t respBodyCap);

#endif
