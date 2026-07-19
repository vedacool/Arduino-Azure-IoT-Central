#include "sas_token.h"
#include "sha256.h"
#include "b64url.h"
#include <stdio.h>
#include <string.h>

size_t build_sas_token(const char *key_b64, const char *resource_uri,
                        unsigned long expiry_epoch_secs,
                        char *out, size_t out_cap) {
    uint8_t key_raw[128];
    size_t key_raw_len = base64_decode(key_b64, strlen(key_b64), key_raw);
    if (key_raw_len == 0 || key_raw_len > sizeof(key_raw)) return 0;

    char uri_enc[256];
    url_encode(resource_uri, uri_enc, sizeof(uri_enc));

    /* string-to-sign = url_encoded(resourceUri) + "\n" + expiry */
    char to_sign[300];
    int n = snprintf(to_sign, sizeof(to_sign), "%s\n%lu", uri_enc, expiry_epoch_secs);
    if (n < 0 || (size_t)n >= sizeof(to_sign)) return 0;

    uint8_t mac[SHA256_BLOCK_SIZE];
    hmac_sha256(key_raw, key_raw_len, (const uint8_t *)to_sign, (size_t)n, mac);

    char sig_b64[64];
    size_t sig_b64_len = base64_encode(mac, SHA256_BLOCK_SIZE, sig_b64);
    sig_b64[sig_b64_len] = '\0';

    char sig_enc[128];
    url_encode(sig_b64, sig_enc, sizeof(sig_enc));

    int written = snprintf(out, out_cap,
        "SharedAccessSignature sr=%s&sig=%s&se=%lu",
        uri_enc, sig_enc, expiry_epoch_secs);
    if (written < 0 || (size_t)written >= out_cap) return 0;
    return (size_t)written;
}
