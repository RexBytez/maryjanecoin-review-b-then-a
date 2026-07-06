#ifndef SECP256K1_CONTRIB_BER_PRIVATEKEY_H
#define SECP256K1_CONTRIB_BER_PRIVATEKEY_H

#ifndef SECP256K1_H
#include <secp256k1.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

SECP256K1_WARN_UNUSED_RESULT int ec_privkey_export_der(
    const secp256k1_context* ctx,
    unsigned char *privkey,
    size_t *privkeylen,
    const unsigned char *seckey,
    int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

SECP256K1_WARN_UNUSED_RESULT int ec_privkey_import_der(
    const secp256k1_context* ctx,
    unsigned char *seckey,
    const unsigned char *privkey,
    size_t privkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

#ifdef __cplusplus
}
#endif

#endif
