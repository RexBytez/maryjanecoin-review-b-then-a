#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>

#include "examples_util.h"

int main(void) {

    unsigned char msg_hash[32] = {
        0x31, 0x5F, 0x5B, 0xDB, 0x76, 0xD0, 0x78, 0xC4,
        0x3B, 0x8A, 0xC0, 0x06, 0x4E, 0x4A, 0x01, 0x64,
        0x61, 0x2B, 0x1F, 0xCE, 0x77, 0xC8, 0x69, 0x34,
        0x5B, 0xFC, 0x94, 0xC7, 0x58, 0x94, 0xED, 0xD3,
    };
    unsigned char seckey[32];
    unsigned char randomize[32];
    unsigned char compressed_pubkey[33];
    unsigned char serialized_signature[64];
    size_t len;
    int is_signature_valid, is_signature_valid2;
    int return_val;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_signature sig;

    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!fill_random(randomize, sizeof(randomize))) {
        printf("Failed to generate randomness\n");
        return 1;
    }

    return_val = secp256k1_context_randomize(ctx, randomize);
    assert(return_val);

    while (1) {
        if (!fill_random(seckey, sizeof(seckey))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        if (secp256k1_ec_seckey_verify(ctx, seckey)) {
            break;
        }
    }

    return_val = secp256k1_ec_pubkey_create(ctx, &pubkey, seckey);
    assert(return_val);

    len = sizeof(compressed_pubkey);
    return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey, &len, &pubkey, SECP256K1_EC_COMPRESSED);
    assert(return_val);

    assert(len == sizeof(compressed_pubkey));

    return_val = secp256k1_ecdsa_sign(ctx, &sig, msg_hash, seckey, NULL, NULL);
    assert(return_val);

    return_val = secp256k1_ecdsa_signature_serialize_compact(ctx, serialized_signature, &sig);
    assert(return_val);

    if (!secp256k1_ecdsa_signature_parse_compact(ctx, &sig, serialized_signature)) {
        printf("Failed parsing the signature\n");
        return 1;
    }

    if (!secp256k1_ec_pubkey_parse(ctx, &pubkey, compressed_pubkey, sizeof(compressed_pubkey))) {
        printf("Failed parsing the public key\n");
        return 1;
    }

    is_signature_valid = secp256k1_ecdsa_verify(ctx, &sig, msg_hash, &pubkey);

    printf("Is the signature valid? %s\n", is_signature_valid ? "true" : "false");
    printf("Secret Key: ");
    print_hex(seckey, sizeof(seckey));
    printf("Public Key: ");
    print_hex(compressed_pubkey, sizeof(compressed_pubkey));
    printf("Signature: ");
    print_hex(serialized_signature, sizeof(serialized_signature));

    secp256k1_context_destroy(ctx);

    is_signature_valid2 = secp256k1_ecdsa_verify(secp256k1_context_static,
                                                 &sig, msg_hash, &pubkey);
    assert(is_signature_valid2 == is_signature_valid);

    secure_erase(seckey, sizeof(seckey));

    return 0;
}
