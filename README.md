# uno_wifi_azure — Arduino Uno WiFi Rev2 → Azure IoT Central

Replacement for firedog1024/arduino-uno-wifi-iotc + Azure/dps-keygen.

## What's different from the GitHub version

| | Old (firedog1024 + dps-keygen) | This sketch |
|---|---|---|
| Getting the IoT Hub connection string | PC tool (`dps_cstr.exe`) — **archived, dead since Jan 2020** | Device provisions itself via DPS over HTTPS at boot. No PC tool. |
| Survives IoT Central reassigning the device to a different hub | No — hardcoded hostname goes stale | Yes — re-provisions and gets the current hub every boot |
| MQTT buffer size | Slides tell you to hand-edit `PubSubClient.h` | `setBufferSize()` call (supported since PubSubClient v2.8) — no library edits |
| String handling | `String` concatenation/replace for JSON payloads | Fixed `char` buffers + `snprintf` — no heap fragmentation over a multi-hour run |
| Crypto | Bundled, unreviewed SHA-256/base64/HMAC | Same algorithms, but verified byte-for-byte against Python's `hashlib`/`hmac` (see Verification below) |

## Files

- `uno_wifi_azure.ino` — setup/loop, WiFi connect, sensor read, MQTT publish
- `dps_client.h/.cpp` — on-device DPS registration (PUT + poll, per Microsoft's documented REST API)
- `sas_token.h/.c` — builds the Azure SAS token string
- `sha256.h/.c`, `b64url.h/.c` — SHA-256, HMAC-SHA256, base64, URL-encoding (the crypto primitives SAS tokens need)
- `config.h` — put your Wi-Fi and IoT Central device credentials here

## One manual step you still have to do (can't be done from the sketch)

Azure completed its move to the **DigiCert Global Root G2** TLS certificate in September 2024. The WiFiNINA module validates TLS using certificates you've explicitly uploaded to it — it does not ship with Azure's current root pre-loaded. Before this sketch can connect:

1. Arduino IDE → **Tools → WiFiNINA Firmware/Certificates Updater**
2. Add these domains, then upload:
   - `global.azure-devices-provisioning.net`
   - your assigned hub's hostname (find it by running the sketch once — it prints "Assigned hub: ..." to Serial — then re-run the certificate updater and add that domain too)

Skipping this step is the single most likely reason a from-scratch Azure/Arduino sketch fails to connect, independent of anything else here.

## Verification performed (and what wasn't)

**Verified — cryptographic core.** `sha256.c`/`b64url.c`/`sas_token.c` were compiled standalone on x86 and checked against:
- NIST test vector `SHA256("abc")`
- RFC 4231 HMAC-SHA256 Test Case 2
- A byte-for-byte comparison of the full SAS-token string against an independent Python (`hashlib`/`hmac`/`base64`) implementation of Azure's documented algorithm, across 8 randomized (key, hub, device, expiry) combinations, including a >64-byte key (the trickiest edge case in HMAC). All matched exactly.

**Verified — API usage.** Every WiFiNINA/PubSubClient call in the sketch was checked against the actual current library source (cloned from `arduino-libraries/WiFiNINA` and `knolleary/pubsubclient`) to confirm the method signatures exist and match.

**Verified — compiles and runs end-to-end.** The full, unmodified sketch source was compiled and linked against hand-written mocks whose method signatures were copied from the real libraries, then run: it connects, builds a correct SAS token, and sends a correctly-formatted HTTPS PUT to the real DPS REST endpoint path with the correct headers. This process caught and fixed two real bugs before you ever touch hardware:
- A missing `#include` (`dps_client.cpp` used `WiFi.getTime()` without including the header that declares it)
- Missing `extern "C"` linkage guards on the crypto headers (would have caused a link error when the C++ sketch called into the plain-C crypto files)

**Not verified — and can't be, in this environment.** I don't have a physical Arduino Uno WiFi Rev2, real Azure IoT Central credentials, or the Arduino-specific `atmega4809` compiler device pack (it's only distributed through Arduino's own Board Manager download server, which this sandbox can't reach). So: the sketch has not been compiled with the exact target toolchain, uploaded to real hardware, or tested against a live Azure endpoint. Before your workshop session, compile it once in the real Arduino IDE (Verify, not Upload, if you want a quick check) and test the DPS connection against one real device.
