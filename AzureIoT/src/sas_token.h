#ifndef SAS_TOKEN_H
#define SAS_TOKEN_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Builds: "SharedAccessSignature sr=<urlenc(resourceUri)>&sig=<urlenc(base64(HMAC))>&se=<expiry>"
   key_b64: device primary/secondary key, base64-encoded, as copied from Azure portal.
   resource_uri: e.g. "myhub.azure-devices.net/devices/mydevice"
   expiry_epoch_secs: token expiry, UNIX seconds.
   out/out_cap: destination buffer.
   Returns length written, or 0 on failure (buffer too small). */
size_t build_sas_token(const char *key_b64, const char *resource_uri,
                        unsigned long expiry_epoch_secs,
                        char *out, size_t out_cap);


#ifdef __cplusplus
}
#endif

#endif
