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
// issue, auth failure, wrong device/telemetry name, non-numeric value --
// without distinguishing which, matching the same simplicity-over-detail
// tradeoff as dpsProvision()'s bool return elsewhere in this library.
bool azureiot_poll_remote_telemetry(const char *appSubdomain, const char *apiToken,
                                     const char *remoteDeviceId, const char *telemetryName,
                                     float *outValue);

#endif
