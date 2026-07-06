#ifndef SECP256K1_ECMULT_CONST_H
#define SECP256K1_ECMULT_CONST_H

#include "scalar.h"
#include "group.h"

static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *q);

static int secp256k1_ecmult_const_xonly(
    secp256k1_fe *r,
    const secp256k1_fe *n,
    const secp256k1_fe *d,
    const secp256k1_scalar *q,
    int known_on_curve
);

#endif
