#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_ecdh.h>

#include "examples_util.h"

int main(void) {
    unsigned char seckey1[32];
    unsigned char seckey2[32];
    unsigned char compressed_pubkey1[33];
    unsigned char compressed_pubkey2[33];
    unsigned char shared_secret1[32];
    unsigned char shared_secret2[32];
    unsigned char randomize[32];
    int return_val;
    size_t len;
    secp256k1_pubkey pubkey1;
    secp256k1_pubkey pubkey2;

    secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    if (!fill_random(randomize, sizeof(randomize))) {
        printf("Failed to generate randomness\n");
        return 1;
    }

    return_val = secp256k1_context_randomize(ctx, randomize);
    assert(return_val);

    while (1) {
        if (!fill_random(seckey1, sizeof(seckey1)) || !fill_random(seckey2, sizeof(seckey2))) {
            printf("Failed to generate randomness\n");
            return 1;
        }
        if (secp256k1_ec_seckey_verify(ctx, seckey1) && secp256k1_ec_seckey_verify(ctx, seckey2)) {
            break;
        }
    }

    return_val = secp256k1_ec_pubkey_create(ctx, &pubkey1, seckey1);
    assert(return_val);
    return_val = secp256k1_ec_pubkey_create(ctx, &pubkey2, seckey2);
    assert(return_val);

    len = sizeof(compressed_pubkey1);
    return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey1, &len, &pubkey1, SECP256K1_EC_COMPRESSED);
    assert(return_val);

    assert(len == sizeof(compressed_pubkey1));

    len = sizeof(compressed_pubkey2);
    return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey2, &len, &pubkey2, SECP256K1_EC_COMPRESSED);
    assert(return_val);

    assert(len == sizeof(compressed_pubkey2));

    return_val = secp256k1_ecdh(ctx, shared_secret1, &pubkey2, seckey1, NULL, NULL);
    assert(return_val);

    return_val = secp256k1_ecdh(ctx, shared_secret2, &pubkey1, seckey2, NULL, NULL);
    assert(return_val);

    return_val = memcmp(shared_secret1, shared_secret2, sizeof(shared_secret1));
    assert(return_val == 0);

    printf("Secret Key1: ");
    print_hex(seckey1, sizeof(seckey1));
    printf("Compressed Pubkey1: ");
    print_hex(compressed_pubkey1, sizeof(compressed_pubkey1));
    printf("\nSecret Key2: ");
    print_hex(seckey2, sizeof(seckey2));
    printf("Compressed Pubkey2: ");
    print_hex(compressed_pubkey2, sizeof(compressed_pubkey2));
    printf("\nShared Secret: ");
    print_hex(shared_secret1, sizeof(shared_secret1));

    secp256k1_context_destroy(ctx);

    secure_erase(seckey1, sizeof(seckey1));
    secure_erase(seckey2, sizeof(seckey2));
    secure_erase(shared_secret1, sizeof(shared_secret1));
    secure_erase(shared_secret2, sizeof(shared_secret2));

    return 0;
}
