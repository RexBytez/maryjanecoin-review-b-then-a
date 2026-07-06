#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#include "examples_util.h"

int main(void) {
    unsigned char msg[12] = "Hello World!";
    unsigned char msg_hash[32];
    unsigned char tag[17] = "my_fancy_protocol";
    unsigned char seckey[32];
    unsigned char randomize[32];
    unsigned char auxiliary_rand[32];
    unsigned char serialized_pubkey[32];
    unsigned char signature[64];
    int is_signature_valid, is_signature_valid2;
    int return_val;
    secp256k1_xonly_pubkey pubkey;
    secp256k1_keypair keypair;

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

        if (secp256k1_keypair_create(ctx, &keypair, seckey)) {
            break;
        }
    }

    return_val = secp256k1_keypair_xonly_pub(ctx, &pubkey, NULL, &keypair);
    assert(return_val);

    return_val = secp256k1_xonly_pubkey_serialize(ctx, serialized_pubkey, &pubkey);
    assert(return_val);

    return_val = secp256k1_tagged_sha256(ctx, msg_hash, tag, sizeof(tag), msg, sizeof(msg));
    assert(return_val);

    if (!fill_random(auxiliary_rand, sizeof(auxiliary_rand))) {
        printf("Failed to generate randomness\n");
        return 1;
    }

    return_val = secp256k1_schnorrsig_sign32(ctx, signature, msg_hash, &keypair, auxiliary_rand);
    assert(return_val);

    if (!secp256k1_xonly_pubkey_parse(ctx, &pubkey, serialized_pubkey)) {
        printf("Failed parsing the public key\n");
        return 1;
    }

    return_val = secp256k1_tagged_sha256(ctx, msg_hash, tag, sizeof(tag), msg, sizeof(msg));
    assert(return_val);

    is_signature_valid = secp256k1_schnorrsig_verify(ctx, signature, msg_hash, 32, &pubkey);

    printf("Is the signature valid? %s\n", is_signature_valid ? "true" : "false");
    printf("Secret Key: ");
    print_hex(seckey, sizeof(seckey));
    printf("Public Key: ");
    print_hex(serialized_pubkey, sizeof(serialized_pubkey));
    printf("Signature: ");
    print_hex(signature, sizeof(signature));

    secp256k1_context_destroy(ctx);

    is_signature_valid2 = secp256k1_schnorrsig_verify(secp256k1_context_static,
                                                      signature, msg_hash, 32, &pubkey);
    assert(is_signature_valid2 == is_signature_valid);

    secure_erase(seckey, sizeof(seckey));
    return 0;
}
