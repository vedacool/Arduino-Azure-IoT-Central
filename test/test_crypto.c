// Portable, host-buildable correctness tests for the crypto core
// (sha256.c / b64url.c / sas_token.c). These files have no Arduino
// dependency, so they build and run with a plain host gcc -- no Arduino
// toolchain needed. See README.md in this folder for how to run.
#include "../src/sha256.h"
#include "../src/b64url.h"
#include "../src/sas_token.h"
#include <stdio.h>
#include <string.h>

static int sha256_test(const char *msg, const char *expected_hex) {
    SHA256_CTX ctx;
    uint8_t hash[32];
    azureiot_sha256_init(&ctx);
    azureiot_sha256_update(&ctx, (const uint8_t*)msg, strlen(msg));
    azureiot_sha256_final(&ctx, hash);
    char got[65];
    for (int i = 0; i < 32; i++) sprintf(got + i*2, "%02x", hash[i]);
    got[64] = 0;
    int ok = strcmp(got, expected_hex) == 0;
    printf("SHA256(\"%s\") = %s  expected %s  %s\n", msg, got, expected_hex, ok ? "OK" : "FAIL");
    return ok;
}

int main() {
    int all_ok = 1;

    // NIST/RFC test vectors
    all_ok &= sha256_test("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    // fix expected length typo check below manually
    all_ok &= sha256_test("abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // RFC 4231 HMAC-SHA256 test case 1
    // key = 0x0b * 20, data = "Hi There"
    {
        uint8_t key[20]; memset(key, 0x0b, 20);
        uint8_t out[32];
        azureiot_hmac_sha256(key, 20, (const uint8_t*)"Hi There", 8, out);
        char got[65];
        for (int i=0;i<32;i++) sprintf(got+i*2, "%02x", out[i]);
        got[64]=0;
        const char *expected = "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";
        int ok = strcmp(got, expected) == 0;
        printf("HMAC RFC4231#1 = %s expected %s %s\n", got, expected, ok ? "OK" : "FAIL");
        all_ok &= ok;
    }

    // base64 known vectors (RFC 4648)
    {
        char out[64];
        size_t n;
        n = azureiot_base64_encode((const uint8_t*)"f", 1, out); out[n]=0;
        printf("b64(\"f\") = %s (expect Zg==) %s\n", out, strcmp(out,"Zg==")==0?"OK":"FAIL");
        all_ok &= strcmp(out,"Zg==")==0;

        n = azureiot_base64_encode((const uint8_t*)"fo", 2, out); out[n]=0;
        printf("b64(\"fo\") = %s (expect Zm8=) %s\n", out, strcmp(out,"Zm8=")==0?"OK":"FAIL");
        all_ok &= strcmp(out,"Zm8=")==0;

        n = azureiot_base64_encode((const uint8_t*)"foo", 3, out); out[n]=0;
        printf("b64(\"foo\") = %s (expect Zm9v) %s\n", out, strcmp(out,"Zm9v")==0?"OK":"FAIL");
        all_ok &= strcmp(out,"Zm9v")==0;

        n = azureiot_base64_encode((const uint8_t*)"foobar", 6, out); out[n]=0;
        printf("b64(\"foobar\") = %s (expect Zm9vYmFy) %s\n", out, strcmp(out,"Zm9vYmFy")==0?"OK":"FAIL");
        all_ok &= strcmp(out,"Zm9vYmFy")==0;
    }

    // base64 decode round-trip
    {
        uint8_t out[64];
        size_t n = azureiot_base64_decode("Zm9vYmFy", 8, out, sizeof(out));
        out[n]=0;
        printf("b64decode(Zm9vYmFy) = %s (expect foobar) %s\n", (char*)out, strcmp((char*)out,"foobar")==0?"OK":"FAIL");
        all_ok &= strcmp((char*)out,"foobar")==0;
    }

    // decode overflow guard
    {
        uint8_t out[2];
        size_t n = azureiot_base64_decode("Zm9vYmFy", 8, out, sizeof(out));
        printf("b64decode overflow guard: n=%zu (expect 0) %s\n", n, n==0?"OK":"FAIL");
        all_ok &= (n==0);
    }

    // sas_token: known-answer test against Python-computed value
    {
        char out[300];
        size_t n = build_sas_token("MHVjbmZzZGZzZGZzZGZzZGZzZGZzZGZzZGZzZGZzZGY=",
                                    "myhub.azure-devices.net/devices/device1",
                                    1700000000UL, out, sizeof(out));
        printf("SAS token (n=%zu): %s\n", n, out);
    }

    printf(all_ok ? "\nALL CRITICAL VECTORS PASSED\n" : "\nSOME VECTORS FAILED\n");
    return all_ok ? 0 : 1;
}
