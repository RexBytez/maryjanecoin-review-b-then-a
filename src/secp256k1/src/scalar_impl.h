#ifndef SECP256K1_SCALAR_IMPL_H
#define SECP256K1_SCALAR_IMPL_H

#ifdef VERIFY
#include <string.h>
#endif

#include "scalar.h"
#include "util.h"

#if defined(EXHAUSTIVE_TEST_ORDER)
#include "scalar_low_impl.h"
#elif defined(SECP256K1_WIDEMUL_INT128)
#include "scalar_4x64_impl.h"
#elif defined(SECP256K1_WIDEMUL_INT64)
#include "scalar_8x32_impl.h"
#else
#error "Please select wide multiplication implementation"
#endif

static const secp256k1_scalar secp256k1_scalar_one = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 1);
static const secp256k1_scalar secp256k1_scalar_zero = SECP256K1_SCALAR_CONST(0, 0, 0, 0, 0, 0, 0, 0);

static int secp256k1_scalar_set_b32_seckey(secp256k1_scalar *r, const unsigned char *bin) {
    int overflow;
    secp256k1_scalar_set_b32(r, bin, &overflow);

    SECP256K1_SCALAR_VERIFY(r);
    return (!overflow) & (!secp256k1_scalar_is_zero(r));
}

static void secp256k1_scalar_verify(const secp256k1_scalar *r) {
    VERIFY_CHECK(secp256k1_scalar_check_overflow(r) == 0);

    (void)r;
}

#if defined(EXHAUSTIVE_TEST_ORDER)

#  if EXHAUSTIVE_TEST_ORDER == 7
#    define EXHAUSTIVE_TEST_LAMBDA 2
#  elif EXHAUSTIVE_TEST_ORDER == 13
#    define EXHAUSTIVE_TEST_LAMBDA 9
#  elif EXHAUSTIVE_TEST_ORDER == 199
#    define EXHAUSTIVE_TEST_LAMBDA 92
#  else
#    error No known lambda for the specified exhaustive test group order.
#  endif

static void secp256k1_scalar_split_lambda(secp256k1_scalar * SECP256K1_RESTRICT r1, secp256k1_scalar * SECP256K1_RESTRICT r2, const secp256k1_scalar * SECP256K1_RESTRICT k) {
    SECP256K1_SCALAR_VERIFY(k);
    VERIFY_CHECK(r1 != k);
    VERIFY_CHECK(r2 != k);
    VERIFY_CHECK(r1 != r2);

    *r2 = (*k + 5) % EXHAUSTIVE_TEST_ORDER;
    *r1 = (*k + (EXHAUSTIVE_TEST_ORDER - *r2) * EXHAUSTIVE_TEST_LAMBDA) % EXHAUSTIVE_TEST_ORDER;

    SECP256K1_SCALAR_VERIFY(r1);
    SECP256K1_SCALAR_VERIFY(r2);
}
#else

static const secp256k1_scalar secp256k1_const_lambda = SECP256K1_SCALAR_CONST(
    0x5363AD4CUL, 0xC05C30E0UL, 0xA5261C02UL, 0x8812645AUL,
    0x122E22EAUL, 0x20816678UL, 0xDF02967CUL, 0x1B23BD72UL
);

#ifdef VERIFY
static void secp256k1_scalar_split_lambda_verify(const secp256k1_scalar *r1, const secp256k1_scalar *r2, const secp256k1_scalar *k);
#endif

static void secp256k1_scalar_split_lambda(secp256k1_scalar * SECP256K1_RESTRICT r1, secp256k1_scalar * SECP256K1_RESTRICT r2, const secp256k1_scalar * SECP256K1_RESTRICT k) {
    secp256k1_scalar c1, c2;
    static const secp256k1_scalar minus_b1 = SECP256K1_SCALAR_CONST(
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL,
        0xE4437ED6UL, 0x010E8828UL, 0x6F547FA9UL, 0x0ABFE4C3UL
    );
    static const secp256k1_scalar minus_b2 = SECP256K1_SCALAR_CONST(
        0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFEUL,
        0x8A280AC5UL, 0x0774346DUL, 0xD765CDA8UL, 0x3DB1562CUL
    );
    static const secp256k1_scalar g1 = SECP256K1_SCALAR_CONST(
        0x3086D221UL, 0xA7D46BCDUL, 0xE86C90E4UL, 0x9284EB15UL,
        0x3DAA8A14UL, 0x71E8CA7FUL, 0xE893209AUL, 0x45DBB031UL
    );
    static const secp256k1_scalar g2 = SECP256K1_SCALAR_CONST(
        0xE4437ED6UL, 0x010E8828UL, 0x6F547FA9UL, 0x0ABFE4C4UL,
        0x221208ACUL, 0x9DF506C6UL, 0x1571B4AEUL, 0x8AC47F71UL
    );
    SECP256K1_SCALAR_VERIFY(k);
    VERIFY_CHECK(r1 != k);
    VERIFY_CHECK(r2 != k);
    VERIFY_CHECK(r1 != r2);

    secp256k1_scalar_mul_shift_var(&c1, k, &g1, 384);
    secp256k1_scalar_mul_shift_var(&c2, k, &g2, 384);
    secp256k1_scalar_mul(&c1, &c1, &minus_b1);
    secp256k1_scalar_mul(&c2, &c2, &minus_b2);
    secp256k1_scalar_add(r2, &c1, &c2);
    secp256k1_scalar_mul(r1, r2, &secp256k1_const_lambda);
    secp256k1_scalar_negate(r1, r1);
    secp256k1_scalar_add(r1, r1, k);

    SECP256K1_SCALAR_VERIFY(r1);
    SECP256K1_SCALAR_VERIFY(r2);
#ifdef VERIFY
    secp256k1_scalar_split_lambda_verify(r1, r2, k);
#endif
}

#ifdef VERIFY

static void secp256k1_scalar_split_lambda_verify(const secp256k1_scalar *r1, const secp256k1_scalar *r2, const secp256k1_scalar *k) {
    secp256k1_scalar s;
    unsigned char buf1[32];
    unsigned char buf2[32];

    static const unsigned char k1_bound[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xa2, 0xa8, 0x91, 0x8c, 0xa8, 0x5b, 0xaf, 0xe2, 0x20, 0x16, 0xd0, 0xb9, 0x17, 0xe4, 0xdd, 0x77
    };

    static const unsigned char k2_bound[32] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x8a, 0x65, 0x28, 0x7b, 0xd4, 0x71, 0x79, 0xfb, 0x2b, 0xe0, 0x88, 0x46, 0xce, 0xa2, 0x67, 0xed
    };

    secp256k1_scalar_mul(&s, &secp256k1_const_lambda, r2);
    secp256k1_scalar_add(&s, &s, r1);
    VERIFY_CHECK(secp256k1_scalar_eq(&s, k));

    secp256k1_scalar_negate(&s, r1);
    secp256k1_scalar_get_b32(buf1, r1);
    secp256k1_scalar_get_b32(buf2, &s);
    VERIFY_CHECK(secp256k1_memcmp_var(buf1, k1_bound, 32) < 0 || secp256k1_memcmp_var(buf2, k1_bound, 32) < 0);

    secp256k1_scalar_negate(&s, r2);
    secp256k1_scalar_get_b32(buf1, r2);
    secp256k1_scalar_get_b32(buf2, &s);
    VERIFY_CHECK(secp256k1_memcmp_var(buf1, k2_bound, 32) < 0 || secp256k1_memcmp_var(buf2, k2_bound, 32) < 0);
}
#endif
#endif

#endif
