#ifndef SECP256K1_ELLSWIFT_H
#define SECP256K1_ELLSWIFT_H

#include "secp256k1.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*secp256k1_ellswift_xdh_hash_function)(
    unsigned char *output,
    const unsigned char *x32,
    const unsigned char *ell_a64,
    const unsigned char *ell_b64,
    void *data
);

SECP256K1_API const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_prefix;

SECP256K1_API const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_bip324;

SECP256K1_API int secp256k1_ellswift_encode(
    const secp256k1_context *ctx,
    unsigned char *ell64,
    const secp256k1_pubkey *pubkey,
    const unsigned char *rnd32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

SECP256K1_API int secp256k1_ellswift_decode(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *ell64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ellswift_create(
    const secp256k1_context *ctx,
    unsigned char *ell64,
    const unsigned char *seckey32,
    const unsigned char *auxrnd32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ellswift_xdh(
  const secp256k1_context *ctx,
  unsigned char *output,
  const unsigned char *ell_a64,
  const unsigned char *ell_b64,
  const unsigned char *seckey32,
  int party,
  secp256k1_ellswift_xdh_hash_function hashfp,
  void *data
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(7);

#ifdef __cplusplus
}
#endif

#endif
