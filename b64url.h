#ifndef B64_URL_H
#define B64_URL_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Returns encoded length written into out (out must be sized via base64_enc_len).
   out is NOT null-terminated by this function; caller null-terminates if needed. */
size_t base64_encode(const uint8_t *in, size_t in_len, char *out);
size_t base64_enc_len(size_t in_len);

/* Decodes base64 text (len chars, no padding requirement assumed to be exact)
   into out. Returns number of decoded bytes. */
size_t base64_decode(const char *in, size_t in_len, uint8_t *out);
size_t base64_dec_len(const char *in, size_t in_len);

/* RFC 3986 percent-encoding, but ALSO treating space as %20 (not '+'), and
   leaving unreserved chars (A-Z a-z 0-9 - _ . ~) untouched. This matches what
   Azure's own JS/Python reference implementations do via encodeURIComponent /
   urllib.parse.quote_plus semantics for the specific characters that appear in
   IoT Hub resource URIs and base64 signatures ('/', '=', '+' all get encoded). */
size_t url_encode(const char *in, char *out, size_t out_cap);


#ifdef __cplusplus
}
#endif

#endif
