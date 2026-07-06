#ifndef SECP256K1_SCALAR_REPR_H
#define SECP256K1_SCALAR_REPR_H

#include <stdint.h>

typedef uint32_t secp256k1_scalar;

#define SCALAR_2P32 ((0xffffffffUL % EXHAUSTIVE_TEST_ORDER) + 1U)

#define SCALAR_HORNER(a, b) (((uint64_t)(a) * SCALAR_2P32 + (b)) % EXHAUSTIVE_TEST_ORDER)

#define SECP256K1_SCALAR_CONST(d7, d6, d5, d4, d3, d2, d1, d0) SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER(SCALAR_HORNER((d7), (d6)), (d5)), (d4)), (d3)), (d2)), (d1)), (d0))

#endif
