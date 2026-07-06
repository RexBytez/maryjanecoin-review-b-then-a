#ifndef SECP256K1_PRECOMPUTED_ECMULT_GEN_H
#define SECP256K1_PRECOMPUTED_ECMULT_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "group.h"
#include "ecmult_gen.h"
#ifdef EXHAUSTIVE_TEST_ORDER
static secp256k1_ge_storage secp256k1_ecmult_gen_prec_table[COMB_BLOCKS][COMB_POINTS];
#else
extern const secp256k1_ge_storage secp256k1_ecmult_gen_prec_table[COMB_BLOCKS][COMB_POINTS];
#endif

#ifdef __cplusplus
}
#endif

#endif
