#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_ellswift.h>

#include "examples_util.h"

int main(void) {
    secp256k1_context* ctx;
    unsigned char randomize[32];
    unsigned char auxrand1[32];
    unsigned char auxrand2[32];
    unsigned char seckey1[32];
    unsigned char seckey2[32];
    unsigned char ellswift_pubkey1[64];
    unsigned char ellswift_pubkey2[64];
    unsigned char shared_secret1[32];
    unsigned char shared_secret2[32];
    int return_val;

    ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
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

    if (!fill_random(auxrand1, sizeof(auxrand1)) || !fill_random(auxrand2, sizeof(auxrand2))) {
        printf("Failed to generate randomness\n");
        return 1;
    }
    return_val = secp256k1_ellswift_create(ctx, ellswift_pubkey1, seckey1, auxrand1);
    assert(return_val);
    return_val = secp256k1_ellswift_create(ctx, ellswift_pubkey2, seckey2, auxrand2);
    assert(return_val);

    return_val = secp256k1_ellswift_xdh(ctx, shared_secret1, ellswift_pubkey1, ellswift_pubkey2,
        seckey1, 0, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
    assert(return_val);

    return_val = secp256k1_ellswift_xdh(ctx, shared_secret2, ellswift_pubkey1, ellswift_pubkey2,
        seckey2, 1, secp256k1_ellswift_xdh_hash_function_bip324, NULL);
    assert(return_val);

    return_val = memcmp(shared_secret1, shared_secret2, sizeof(shared_secret1));
    assert(return_val == 0);

    printf(  "     Secret Key1: ");
    print_hex(seckey1, sizeof(seckey1));
    printf(  "EllSwift Pubkey1: ");
    print_hex(ellswift_pubkey1, sizeof(ellswift_pubkey1));
    printf("\n     Secret Key2: ");
    print_hex(seckey2, sizeof(seckey2));
    printf(  "EllSwift Pubkey2: ");
    print_hex(ellswift_pubkey2, sizeof(ellswift_pubkey2));
    printf("\n   Shared Secret: ");
    print_hex(shared_secret1, sizeof(shared_secret1));

    secp256k1_context_destroy(ctx);

    secure_erase(seckey1, sizeof(seckey1));
    secure_erase(seckey2, sizeof(seckey2));
    secure_erase(shared_secret1, sizeof(shared_secret1));
    secure_erase(shared_secret2, sizeof(shared_secret2));

    return 0;
}
