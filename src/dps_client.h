#ifndef DPS_CLIENT_H
#define DPS_CLIENT_H
#include <Arduino.h>
#include <WiFiNINA.h>
#include <WiFiSSLClient.h>

// Result of a successful DPS provisioning round.
struct DpsResult {
    char assignedHub[128];   // e.g. "iotc-xxxxxxxx-....azure-devices.net"
    char deviceId[64];
};

// Performs the full DPS individual-enrollment (symmetric key) flow:
//   PUT  .../{idScope}/registrations/{deviceId}/register?api-version=2019-03-31
//   GET  .../{idScope}/registrations/{deviceId}/operations/{operationId}?... (poll)
// using a SAS token scoped to "{idScope}/registrations/{deviceId}".
// Returns true and fills `out` on success.
bool dpsProvision(const char *idScope, const char *deviceId, const char *deviceKeyB64,
                   DpsResult &out);

#endif
