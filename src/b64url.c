#include "b64url.h"
#include <string.h>

static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t azureiot_base64_enc_len(size_t in_len) {
    return ((in_len + 2) / 3) * 4;
}

size_t azureiot_base64_encode(const uint8_t *in, size_t in_len, char *out) {
    size_t i, o = 0;
    for (i = 0; i + 2 < in_len; i += 3) {
        uint32_t n = ((uint32_t)in[i] << 16) | ((uint32_t)in[i+1] << 8) | in[i+2];
        out[o++] = b64_table[(n >> 18) & 0x3F];
        out[o++] = b64_table[(n >> 12) & 0x3F];
        out[o++] = b64_table[(n >> 6) & 0x3F];
        out[o++] = b64_table[n & 0x3F];
    }
    size_t rem = in_len - i;
    if (rem == 1) {
        uint32_t n = ((uint32_t)in[i] << 16);
        out[o++] = b64_table[(n >> 18) & 0x3F];
        out[o++] = b64_table[(n >> 12) & 0x3F];
        out[o++] = '=';
        out[o++] = '=';
    } else if (rem == 2) {
        uint32_t n = ((uint32_t)in[i] << 16) | ((uint32_t)in[i+1] << 8);
        out[o++] = b64_table[(n >> 18) & 0x3F];
        out[o++] = b64_table[(n >> 12) & 0x3F];
        out[o++] = b64_table[(n >> 6) & 0x3F];
        out[o++] = '=';
    }
    return o;
}

static int8_t b64_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

size_t azureiot_base64_dec_len(const char *in, size_t in_len) {
    if (in_len < 4) return 0; // a valid base64 block is >=4 chars; guards the
                              // (in_len/4)*3 - pad subtraction below from
                              // underflowing to SIZE_MAX on a too-short input
    size_t pad = 0;
    if (in_len >= 1 && in[in_len-1] == '=') pad++;
    if (in_len >= 2 && in[in_len-2] == '=') pad++;
    return (in_len / 4) * 3 - pad;
}

size_t azureiot_base64_decode(const char *in, size_t in_len, uint8_t *out, size_t out_cap) {
    size_t i, o = 0;
    uint32_t buf = 0;
    int bits = 0;
    for (i = 0; i < in_len; ++i) {
        if (in[i] == '=' || in[i] == '\0') break;
        int8_t v = b64_val(in[i]);
        if (v < 0) continue;
        buf = (buf << 6) | (uint32_t)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (o >= out_cap) return 0; /* would overflow caller's buffer */
            out[o++] = (uint8_t)((buf >> bits) & 0xFF);
        }
    }
    return o;
}

static int is_unreserved(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
        return 1;
    switch (c) {
        case '-': case '_': case '.': case '!': case '~':
        case '*': case '\'': case '(': case ')':
            return 1;
        default:
            return 0;
    }
}

static char hex_digit(uint8_t v) {
    return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

size_t azureiot_url_encode(const char *in, char *out, size_t out_cap) {
    size_t o = 0;
    for (const char *p = in; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (is_unreserved((char)c)) {
            if (o + 1 >= out_cap) break;
            out[o++] = (char)c;
        } else {
            if (o + 3 >= out_cap) break;
            out[o++] = '%';
            out[o++] = hex_digit((c >> 4) & 0xF);
            out[o++] = hex_digit(c & 0xF);
        }
    }
    if (o < out_cap) out[o] = '\0';
    return o;
}
