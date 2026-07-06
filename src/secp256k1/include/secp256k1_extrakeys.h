#ifndef SECP256K1_EXTRAKEYS_H
#define SECP256K1_EXTRAKEYS_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char data[64];
} secp256k1_xonly_pubkey;

typedef struct {
    unsigned char data[96];
} secp256k1_keypair;

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_xonly_pubkey_parse(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *pubkey,
    const unsigned char *input32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API int secp256k1_xonly_pubkey_serialize(
    const secp256k1_context *ctx,
    unsigned char *output32,
    const secp256k1_xonly_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API int secp256k1_xonly_pubkey_cmp(
    const secp256k1_context *ctx,
    const secp256k1_xonly_pubkey *pk1,
    const secp256k1_xonly_pubkey *pk2
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_xonly_pubkey_from_pubkey(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *xonly_pubkey,
    int *pk_parity,
    const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_xonly_pubkey_tweak_add(
    const secp256k1_context *ctx,
    secp256k1_pubkey *output_pubkey,
    const secp256k1_xonly_pubkey *internal_pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_xonly_pubkey_tweak_add_check(
    const secp256k1_context *ctx,
    const unsigned char *tweaked_pubkey32,
    int tweaked_pk_parity,
    const secp256k1_xonly_pubkey *internal_pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_keypair_create(
    const secp256k1_context *ctx,
    secp256k1_keypair *keypair,
    const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_keypair_sec(
    const secp256k1_context *ctx,
    unsigned char *seckey,
    const secp256k1_keypair *keypair
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_keypair_pub(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const secp256k1_keypair *keypair
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_keypair_xonly_pub(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *pubkey,
    int *pk_parity,
    const secp256k1_keypair *keypair
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_keypair_xonly_tweak_add(
    const secp256k1_context *ctx,
    secp256k1_keypair *keypair,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

#ifdef __cplusplus
}
#endif

#endif
