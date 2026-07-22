#ifndef AZURE_IOT_REMOTE_TELEMETRY_H
#define AZURE_IOT_REMOTE_TELEMETRY_H
#include <stddef.h>

// Polls the last known value of one telemetry field from ANOTHER device via
// the IoT Central REST API (not this device's own MQTT connection -- a
// completely separate HTTPS request to a different Azure endpoint). See
// AzureIoT.h's onRemoteTelemetry() for the full design and why this exists
// as polling rather than a push, like onBoolProperty().
//
// Returns true and fills *outValue on success (HTTP 200 with a parseable
// "value" field in the response). Returns false on any failure -- network
// issue, auth failure, wrong device/telemetry name, non-numeric value.
//
// outStatusCode (optional, pass nullptr to ignore) is filled with the raw
// HTTP status code on any response received (401 = bad API token, 404 =
// wrong device ID/telemetry name, etc.), or -1 specifically if the HTTPS
// connection itself never completed (a stronger signal of a network/
// hardware-level problem than an HTTP-level error response) -- added after
// real hardware testing showed a plain true/false wasn't enough to tell
// "the request failed at the network level" apart from "Azure responded
// but said no."
bool azureiot_poll_remote_telemetry(const char *appSubdomain, const char *apiToken,
                                     const char *remoteDeviceId, const char *telemetryName,
                                     float *outValue, int *outStatusCode = nullptr);

#endif
