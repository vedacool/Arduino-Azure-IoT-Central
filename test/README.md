# Tests

A plain host-buildable C test for the crypto core (`sha256.c` / `b64url.c` /
`sas_token.c`) -- no Arduino toolchain needed, just a regular `gcc`/`clang`.
These three files have no Arduino dependency, so they compile and run
directly on your machine, testing the exact same code that ships in the
library (not a copy).

It checks SHA-256 and HMAC-SHA256 against published RFC 4231 / RFC 4648 /
NIST test vectors, base64 encode/decode (including the decode overflow
guard), and cross-checks a full SAS token against an independent Python
implementation.

Arduino IDE never touches this folder -- it's not part of the sketch build.
It's here for whoever next needs to verify the crypto core still does the
right thing after a change (that might be you, a future contributor, or an
AI assistant auditing the repo) without having to reconstruct these checks
from scratch.

## Running

```sh
cd test
gcc -Wall -Wextra -fsanitize=address,undefined -o test_crypto test_crypto.c ../src/sha256.c ../src/b64url.c ../src/sas_token.c
./test_crypto
```

Should print a series of `OK` lines ending in `ALL CRITICAL VECTORS PASSED`,
with no ASan/UBSan reports.
