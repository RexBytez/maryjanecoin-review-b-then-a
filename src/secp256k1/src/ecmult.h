#ifndef SECP256K1_ECMULT_H
#define SECP256K1_ECMULT_H

#include "group.h"
#include "scalar.h"
#include "scratch.h"

#ifndef ECMULT_WINDOW_SIZE
#  define ECMULT_WINDOW_SIZE 15
#  ifdef DEBUG_CONFIG
#     pragma message DEBUG_CONFIG_MSG("ECMULT_WINDOW_SIZE undefined, assuming default value")
#  endif
#endif

#ifdef DEBUG_CONFIG
#  pragma message DEBUG_CONFIG_DEF(ECMULT_WINDOW_SIZE)
#endif

#if ECMULT_WINDOW_SIZE < 2 || ECMULT_WINDOW_SIZE > 24
#  error Set ECMULT_WINDOW_SIZE to an integer in range [2..24].
#endif

#define ECMULT_TABLE_SIZE(w) (1L << ((w)-2))

static void secp256k1_ecmult(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_scalar *na, const secp256k1_scalar *ng);

typedef int (secp256k1_ecmult_multi_callback)(secp256k1_scalar *sc, secp256k1_ge *pt, size_t idx, void *data);

static int secp256k1_ecmult_multi_var(const secp256k1_callback* error_callback, secp256k1_scratch *scratch, secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n);

#endif
