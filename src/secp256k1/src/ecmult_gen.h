#ifndef SECP256K1_ECMULT_GEN_H
#define SECP256K1_ECMULT_GEN_H

#include "scalar.h"
#include "group.h"

#if defined(EXHAUSTIVE_TEST_ORDER)

#  undef COMB_BLOCKS
#  undef COMB_TEETH
#  if EXHAUSTIVE_TEST_ORDER == 7
#    define COMB_RANGE 3
#    define COMB_BLOCKS 1
#    define COMB_TEETH 2
#  elif EXHAUSTIVE_TEST_ORDER == 13
#    define COMB_RANGE 4
#    define COMB_BLOCKS 1
#    define COMB_TEETH 2
#  elif EXHAUSTIVE_TEST_ORDER == 199
#    define COMB_RANGE 8
#    define COMB_BLOCKS 2
#    define COMB_TEETH 3
#  else
#    error "Unknown exhaustive test order"
#  endif
#  if (COMB_RANGE >= 32) || ((EXHAUSTIVE_TEST_ORDER >> (COMB_RANGE - 1)) != 1)
#    error "COMB_RANGE != ceil(log2(EXHAUSTIVE_TEST_ORDER+1))"
#  endif
#else
#  define COMB_RANGE 256
#endif

#ifndef COMB_BLOCKS
#  define COMB_BLOCKS 11
#  ifdef DEBUG_CONFIG
#    pragma message DEBUG_CONFIG_MSG("COMB_BLOCKS undefined, assuming default value")
#  endif
#endif
#ifndef COMB_TEETH
#  define COMB_TEETH 6
#  ifdef DEBUG_CONFIG
#    pragma message DEBUG_CONFIG_MSG("COMB_TEETH undefined, assuming default value")
#  endif
#endif

#define COMB_SPACING CEIL_DIV(COMB_RANGE, COMB_BLOCKS * COMB_TEETH)

#define COMB_BITS (COMB_BLOCKS * COMB_TEETH * COMB_SPACING)

#define COMB_POINTS (1 << (COMB_TEETH - 1))

#if !(1 <= COMB_BLOCKS && COMB_BLOCKS <= 256)
#  error "COMB_BLOCKS must be in the range [1, 256]"
#endif
#if !(1 <= COMB_TEETH && COMB_TEETH <= 8)
#  error "COMB_TEETH must be in the range [1, 8]"
#endif
#if COMB_BITS < COMB_RANGE
#  error "COMB_BLOCKS * COMB_TEETH * COMB_SPACING is too low"
#endif

#if (COMB_BLOCKS - 1) * COMB_TEETH * COMB_SPACING >= 256
#  error "COMB_BLOCKS can be reduced"
#endif
#if COMB_BLOCKS * (COMB_TEETH - 1) * COMB_SPACING >= 256
#  error "COMB_TEETH can be reduced"
#endif

#ifdef DEBUG_CONFIG
#  pragma message DEBUG_CONFIG_DEF(COMB_RANGE)
#  pragma message DEBUG_CONFIG_DEF(COMB_BLOCKS)
#  pragma message DEBUG_CONFIG_DEF(COMB_TEETH)
#  pragma message DEBUG_CONFIG_DEF(COMB_SPACING)
#endif

typedef struct {

    int built;

    secp256k1_scalar scalar_offset;
    secp256k1_ge ge_offset;

    secp256k1_fe proj_blind;
} secp256k1_ecmult_gen_context;

static void secp256k1_ecmult_gen_context_build(secp256k1_ecmult_gen_context* ctx);
static void secp256k1_ecmult_gen_context_clear(secp256k1_ecmult_gen_context* ctx);

static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context* ctx, secp256k1_gej *r, const secp256k1_scalar *a);

static void secp256k1_ecmult_gen_blind(secp256k1_ecmult_gen_context *ctx, const unsigned char *seed32);

#endif
