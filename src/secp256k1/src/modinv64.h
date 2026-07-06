#ifndef SECP256K1_MODINV64_H
#define SECP256K1_MODINV64_H

#include "util.h"

#ifndef SECP256K1_WIDEMUL_INT128
#error "modinv64 requires 128-bit wide multiplication support"
#endif

typedef struct {
    int64_t v[5];
} secp256k1_modinv64_signed62;

typedef struct {

    secp256k1_modinv64_signed62 modulus;

    uint64_t modulus_inv62;
} secp256k1_modinv64_modinfo;

static void secp256k1_modinv64_var(secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo);

static void secp256k1_modinv64(secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo);

static int secp256k1_jacobi64_maybe_var(const secp256k1_modinv64_signed62 *x, const secp256k1_modinv64_modinfo *modinfo);

#endif
