#ifndef AZURE_ROOT_CA_H
#define AZURE_ROOT_CA_H

// TLS root certificate(s) Azure IoT Hub/DPS present, needed on boards that
// don't manage their own trusted-root store (ESP32's WiFiClientSecure needs
// this handed to it explicitly via setCACert() -- unlike WiFiNINA boards,
// which get their trust store loaded once via the Arduino IDE's
// Firmware/Certificates Updater tool instead).
//
// Contains DigiCert Global Root G2 (the certificate Azure IoT Hub/DPS has
// used since completing its migration in Sept 2024) plus the Microsoft RSA
// Root Certificate Authority 2017 (recommended for forward compatibility).
// Sourced directly from Microsoft's own official Azure IoT C SDK
// (github.com/Azure/azure-iot-sdk-c, certs/certs.c), not reconstructed by
// hand -- a single wrong byte in a certificate fails silently and
// confusingly, so this is copied verbatim from that authoritative source.
extern const char AZURE_ROOT_CA[];

#endif
