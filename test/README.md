# Tests

These are plain host-buildable C tests -- no Arduino toolchain needed, just
a regular `gcc`/`clang`. They exist so the correctness work already done on
this library (RFC/NIST test vectors, an independent Python cross-check,
AddressSanitizer runs) can be re-run by anyone, instead of living only in
commit-message prose.

## Running

```sh
cd test

# Crypto core (sha256 / base64 / SAS token) against RFC 4648 / RFC 4231 /
# NIST test vectors, plus a base64-decode overflow-guard check.
gcc -Wall -Wextra -fsanitize=address,undefined -o test_crypto test_crypto.c ../src/sha256.c ../src/b64url.c ../src/sas_token.c
./test_crypto

# formatFloat2dp() edge cases (NaN/Infinity, rounding, negative zero).
gcc -Wall -Wextra -fsanitize=address,undefined -o test_format test_format.c -lm
./test_format

# The publish()/flush() JSON payload buffer, adversarially filled with long
# keys, checked for overflow under ASan.
gcc -Wall -Wextra -fsanitize=address,undefined -o test_flush_buffer test_flush_buffer.c -lm
./test_flush_buffer
```

All three should print `OK`/pass lines and exit 0 with no ASan/UBSan
reports.

## A note on `test_format.c` / `test_flush_buffer.c`

`sha256.c`/`b64url.c`/`sas_token.c` are real portable files also compiled
directly into the library, so `test_crypto.c` tests the actual production
code with zero drift risk.

`formatFloat2dp()` and the `appendField()`/`appendJsonEscaped()` payload
logic, on the other hand, live as `static` (file-local) functions inside
`src/AzureIoT.cpp`, which also pulls in Arduino/`PubSubClient` headers --
so they can't be `#include`d directly into a host-only test. `test_format.c`
and `test_flush_buffer.c` instead contain hand-synced copies of that logic.
**If you change the real versions in `AzureIoT.cpp`, update these copies to
match**, or the tests will quietly test stale logic. (A cleaner long-term
fix would be to move these pure functions into their own portable
`.c`/`.h` pair, the same way `sha256.c` etc. already are -- happy to do
that refactor if useful, it just wasn't done here to keep this fix scoped.)
