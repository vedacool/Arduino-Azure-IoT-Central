#ifndef B64_URL_H
#define B64_URL_H
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Returns encoded length written into out (out must be sized via azureiot_base64_enc_len).
   out is NOT null-terminated by this function; caller null-terminates if needed. */
size_t azureiot_base64_encode(const uint8_t *in, size_t in_len, char *out);
size_t azureiot_base64_enc_len(size_t in_len);

/* Decodes base64 text (in_len chars) into out, writing at most out_cap bytes.
   Returns number of decoded bytes, or 0 if the decoded output would not fit
   in out_cap (caller's buffer is left partially written in that case --
   always check the return value before trusting `out`). */
size_t azureiot_base64_decode(const char *in, size_t in_len, uint8_t *out, size_t out_cap);
size_t azureiot_base64_dec_len(const char *in, size_t in_len);

/* RFC 3986 percent-encoding, but ALSO treating space as %20 (not '+'), and
   leaving unreserved chars (A-Z a-z 0-9 - _ . ~) untouched. This matches what
   Azure's own JS/Python reference implementations do via encodeURIComponent /
   urllib.parse.quote_plus semantics for the specific characters that appear in
   IoT Hub resource URIs and base64 signatures ('/', '=', '+' all get encoded). */
size_t azureiot_url_encode(const char *in, char *out, size_t out_cap);


#ifdef __cplusplus
}
#endif

#endif
