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
- `config.h.example` — copy this to `config.h` and fill in your Wi-Fi and IoT Central device credentials. **`config.h` itself is gitignored on purpose — see the audit below.**

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
- A byte-for-byte comparison of the full SAS-token string against an independent Python (`hashlib`/`hmac`/`base64`) implementation of Azure's documented algorithm, across 16 randomized (key, hub, device, expiry) combinations across two test rounds, including >64-byte keys (the trickiest edge case in HMAC). All matched exactly.

**Verified — API usage.** Every WiFiNINA/PubSubClient call in the sketch was checked against the actual current library source (cloned from `arduino-libraries/WiFiNINA` and `knolleary/pubsubclient`) to confirm the method signatures exist and match.

**Verified — compiles and runs end-to-end.** The full, unmodified sketch source was compiled and linked against hand-written mocks whose method signatures were copied from the real libraries, then run.

**Not verified — and can't be, in this environment.** No physical Arduino Uno WiFi Rev2, no real Azure IoT Central credentials, and the Arduino-specific `atmega4809` compiler device pack is only distributed through Arduino's own Board Manager download server, which this sandbox can't reach. Compile it once in the real Arduino IDE and test DPS against one real device before the workshop.

## Post-push code audit (fixed)

A full read-through after the first push found and fixed four real issues:

1. **Buffer overflow in `base64_decode`.** The original signature had no output-capacity parameter — it decoded and wrote as many bytes as the input implied, and the caller's size check happened *after* the write. A key pasted into `config.h` that was much longer than expected (e.g. a whole connection string dropped in by mistake instead of just the key) would have overflowed a 128-byte stack buffer. Fixed by adding an `out_cap` parameter that the function now enforces, returning 0 (and writing nothing further) if the decoded output wouldn't fit. Re-verified with `-fstack-protector-all` and a 500-byte garbage-key test: it now fails cleanly instead of writing past the buffer.
2. **`String` usage in `dps_client.cpp`'s HTTP response parsing** contradicted the sketch's own stated no-heap-allocation design (every `readStringUntil()` call allocates). Replaced with a fixed-buffer line reader (`readLine()`), consistent with the rest of the sketch. Also fails faster and more predictably on a bad/slow connection than the old `String`-based version did in testing.
3. **Dead config constant.** `config.h` defined `DPS_GLOBAL_HOST`, but `dps_client.cpp` silently ignored it and hardcoded its own private copy of the same string. Harmless today (both were identical), but meant that pointing this at Azure Government or Azure China's different DPS endpoints would have had no effect. Fixed — there's now exactly one definition.
4. **`config.h` was tracked in git.** It only held placeholder values, but the moment a student fills in a real Wi-Fi password and Azure device key and runs `git add -A` (a very normal thing to do while iterating), those credentials would be committed. Renamed the tracked file to `config.h.example` and removed `config.h` from tracking (`git rm --cached`), with the actual `config.h` now gitignored.

**Flagged, not changed (judgment calls for you, not bugs):**
- **Peak stack depth.** `dpsProvision()` → `build_sas_token()` → `hmac_sha256()` stacks roughly 1.2–1.5KB of local buffers at their deepest point, on a board with 6KB total SRAM. It ran fine in the mocked test, but there's no headroom measurement on real hardware yet — worth checking `freeMemory()`-style diagnostics during your first real test.
- **No backoff on MQTT reconnect.** If Wi-Fi/Azure is down for a while, `loop()` retries the MQTT connect on every iteration with no cooldown. Fine for a workshop, worth adding a delay if this ever runs unattended for days.
- **No watchdog timer**, and `connectWiFi()`'s Wi-Fi-connect loop has no timeout — wrong SSID/password means it spins on dots forever rather than failing visibly.
- **`readSensors()` doesn't guard against `analogRead()` returning 0** (division by zero → the thermistor math degrades to a non-crashing but meaningless value rather than an error). Inherited from the original workshop slides' sensor code, not introduced here.
- **`extractJsonString()` is a substring search, not a real JSON parser.** It works for the DPS response shape observed in Microsoft's documented examples, but isn't robust if Azure ever nests fields differently.
- The board's **ATECC608 crypto chip goes unused** — this design does the SAS-token HMAC in software. Using the chip for X.509-based auth instead of a symmetric-key SAS token would be more secure but is a bigger redesign, out of scope here.

