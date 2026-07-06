#ifndef SECP256K1_FIELD_REPR_H
#define SECP256K1_FIELD_REPR_H

#include <stdint.h>

typedef struct {

    uint64_t n[5];

    SECP256K1_FE_VERIFY_FIELDS
} secp256k1_fe;

#define SECP256K1_FE_CONST_INNER(d7, d6, d5, d4, d3, d2, d1, d0) { \
    (d0) | (((uint64_t)(d1) & 0xFFFFFUL) << 32), \
    ((uint64_t)(d1) >> 20) | (((uint64_t)(d2)) << 12) | (((uint64_t)(d3) & 0xFFUL) << 44), \
    ((uint64_t)(d3) >> 8) | (((uint64_t)(d4) & 0xFFFFFFFUL) << 24), \
    ((uint64_t)(d4) >> 28) | (((uint64_t)(d5)) << 4) | (((uint64_t)(d6) & 0xFFFFUL) << 36), \
    ((uint64_t)(d6) >> 16) | (((uint64_t)(d7)) << 16) \
}

typedef struct {
    uint64_t n[4];
} secp256k1_fe_storage;

#define SECP256K1_FE_STORAGE_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {{ \
    (d0) | (((uint64_t)(d1)) << 32), \
    (d2) | (((uint64_t)(d3)) << 32), \
    (d4) | (((uint64_t)(d5)) << 32), \
    (d6) | (((uint64_t)(d7)) << 32) \
}}

#define SECP256K1_FE_STORAGE_CONST_GET(d) \
    (uint32_t)(d.n[3] >> 32), (uint32_t)d.n[3], \
    (uint32_t)(d.n[2] >> 32), (uint32_t)d.n[2], \
    (uint32_t)(d.n[1] >> 32), (uint32_t)d.n[1], \
    (uint32_t)(d.n[0] >> 32), (uint32_t)d.n[0]

#endif
