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
//
// outTimestamp (optional, pass nullptr to ignore) is filled with Azure's own
// ISO-8601 timestamp string for when the returned value was actually
// recorded -- e.g. "2026-07-22T15:00:09.059Z". Added after real hardware
// testing raised a fair question ("is this actually the latest value, or
// something stale?") that a bare number couldn't answer -- comparing this
// against your own device's current time is the real, checkable way to know
// how fresh a given poll's value actually is, rather than guessing.
// outTimestampCap is the size of that buffer -- checked, not assumed, same
// as every fixed buffer elsewhere in this library.
bool azureiot_poll_remote_telemetry(const char *appSubdomain, const char *apiToken,
                                     const char *remoteDeviceId, const char *telemetryName,
                                     float *outValue, int *outStatusCode = nullptr,
                                     char *outTimestamp = nullptr, size_t outTimestampCap = 0);

#endif
