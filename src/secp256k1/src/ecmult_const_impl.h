#ifndef SECP256K1_ECMULT_CONST_IMPL_H
#define SECP256K1_ECMULT_CONST_IMPL_H

#include "scalar.h"
#include "group.h"
#include "ecmult_const.h"
#include "ecmult_impl.h"

#if defined(EXHAUSTIVE_TEST_ORDER)

#  if EXHAUSTIVE_TEST_ORDER == 199
#    define ECMULT_CONST_GROUP_SIZE 4
#  elif EXHAUSTIVE_TEST_ORDER == 13
#    define ECMULT_CONST_GROUP_SIZE 3
#  elif EXHAUSTIVE_TEST_ORDER == 7
#    define ECMULT_CONST_GROUP_SIZE 2
#  else
#    error "Unknown EXHAUSTIVE_TEST_ORDER"
#  endif
#else

#  define ECMULT_CONST_GROUP_SIZE 5
#endif

#define ECMULT_CONST_TABLE_SIZE (1L << (ECMULT_CONST_GROUP_SIZE - 1))
#define ECMULT_CONST_GROUPS ((129 + ECMULT_CONST_GROUP_SIZE - 1) / ECMULT_CONST_GROUP_SIZE)
#define ECMULT_CONST_BITS (ECMULT_CONST_GROUPS * ECMULT_CONST_GROUP_SIZE)

static void secp256k1_ecmult_const_odd_multiples_table_globalz(secp256k1_ge *pre, secp256k1_fe *globalz, const secp256k1_gej *a) {
    secp256k1_fe zr[ECMULT_CONST_TABLE_SIZE];

    secp256k1_ecmult_odd_multiples_table(ECMULT_CONST_TABLE_SIZE, pre, zr, globalz, a);
    secp256k1_ge_table_set_globalz(ECMULT_CONST_TABLE_SIZE, pre, zr);
}

#define ECMULT_CONST_TABLE_GET_GE(r,pre,n) do { \
    unsigned int m = 0; \
     \
    volatile unsigned int negative = ((n) >> (ECMULT_CONST_GROUP_SIZE - 1)) ^ 1; \
     \
    unsigned int index = ((unsigned int)(-negative) ^ n) & ((1U << (ECMULT_CONST_GROUP_SIZE - 1)) - 1U); \
    secp256k1_fe neg_y; \
    VERIFY_CHECK((n) < (1U << ECMULT_CONST_GROUP_SIZE)); \
    VERIFY_CHECK(index < (1U << (ECMULT_CONST_GROUP_SIZE - 1))); \
     \
    (r)->x = (pre)[m].x; \
    (r)->y = (pre)[m].y; \
    for (m = 1; m < ECMULT_CONST_TABLE_SIZE; m++) { \
         \
        secp256k1_fe_cmov(&(r)->x, &(pre)[m].x, m == index); \
        secp256k1_fe_cmov(&(r)->y, &(pre)[m].y, m == index); \
    } \
    (r)->infinity = 0; \
    secp256k1_fe_negate(&neg_y, &(r)->y, 1); \
    secp256k1_fe_cmov(&(r)->y, &neg_y, negative); \
} while(0)

#ifdef EXHAUSTIVE_TEST_ORDER
static const secp256k1_scalar secp256k1_ecmult_const_K = ((SECP256K1_SCALAR_CONST(0, 0, 0, (1U << (ECMULT_CONST_BITS - 128)) - 2U, 0, 0, 0, 0) + EXHAUSTIVE_TEST_ORDER - 1U) * (1U + EXHAUSTIVE_TEST_LAMBDA)) % EXHAUSTIVE_TEST_ORDER;

#elif ECMULT_CONST_BITS == 129

static const secp256k1_scalar secp256k1_ecmult_const_K = SECP256K1_SCALAR_CONST(0xac9c52b3ul, 0x3fa3cf1ful, 0x5ad9e3fdul, 0x77ed9ba4ul, 0xa880b9fcul, 0x8ec739c2ul, 0xe0cfc810ul, 0xb51283ceul);
#elif ECMULT_CONST_BITS == 130

static const secp256k1_scalar secp256k1_ecmult_const_K = SECP256K1_SCALAR_CONST(0xa4e88a7dul, 0xcb13034eul, 0xc2bdd6bful, 0x7c118d6bul, 0x589ae848ul, 0x26ba29e4ul, 0xb5c2c1dcul, 0xde9798d9ul);
#elif ECMULT_CONST_BITS == 132

static const secp256k1_scalar secp256k1_ecmult_const_K = SECP256K1_SCALAR_CONST(0x76b1d93dul, 0x0fae3c6bul, 0x3215874bul, 0x94e93813ul, 0x7937fe0dul, 0xb66bcaaful, 0xb3749ca5ul, 0xd7b6171bul);
#else
#  error "Unknown ECMULT_CONST_BITS"
#endif

static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *q) {

    static const secp256k1_scalar S_OFFSET = SECP256K1_SCALAR_CONST(0, 0, 0, 1, 0, 0, 0, 0);
    secp256k1_scalar s, v1, v2;
    secp256k1_ge pre_a[ECMULT_CONST_TABLE_SIZE];
    secp256k1_ge pre_a_lam[ECMULT_CONST_TABLE_SIZE];
    secp256k1_fe global_z;
    int group, i;

    if (secp256k1_ge_is_infinity(a)) {
        secp256k1_gej_set_infinity(r);
        return;
    }

    secp256k1_scalar_add(&s, q, &secp256k1_ecmult_const_K);
    secp256k1_scalar_half(&s, &s);
    secp256k1_scalar_split_lambda(&v1, &v2, &s);
    secp256k1_scalar_add(&v1, &v1, &S_OFFSET);
    secp256k1_scalar_add(&v2, &v2, &S_OFFSET);

#ifdef VERIFY

    for (i = 129; i < 256; ++i) {
        VERIFY_CHECK(secp256k1_scalar_get_bits_limb32(&v1, i, 1) == 0);
        VERIFY_CHECK(secp256k1_scalar_get_bits_limb32(&v2, i, 1) == 0);
    }
#endif

    secp256k1_gej_set_ge(r, a);
    secp256k1_ecmult_const_odd_multiples_table_globalz(pre_a, &global_z, r);
    for (i = 0; i < ECMULT_CONST_TABLE_SIZE; i++) {
        secp256k1_ge_mul_lambda(&pre_a_lam[i], &pre_a[i]);
    }

    for (group = ECMULT_CONST_GROUPS - 1; group >= 0; --group) {

        unsigned int bits1 = secp256k1_scalar_get_bits_var(&v1, group * ECMULT_CONST_GROUP_SIZE, ECMULT_CONST_GROUP_SIZE);
        unsigned int bits2 = secp256k1_scalar_get_bits_var(&v2, group * ECMULT_CONST_GROUP_SIZE, ECMULT_CONST_GROUP_SIZE);
        secp256k1_ge t;
        int j;

        ECMULT_CONST_TABLE_GET_GE(&t, pre_a, bits1);
        if (group == ECMULT_CONST_GROUPS - 1) {

            secp256k1_gej_set_ge(r, &t);
        } else {

            for (j = 0; j < ECMULT_CONST_GROUP_SIZE; ++j) {
                secp256k1_gej_double(r, r);
            }
            secp256k1_gej_add_ge(r, r, &t);
        }
        ECMULT_CONST_TABLE_GET_GE(&t, pre_a_lam, bits2);
        secp256k1_gej_add_ge(r, r, &t);
    }

    secp256k1_fe_mul(&r->z, &r->z, &global_z);
}

static int secp256k1_ecmult_const_xonly(secp256k1_fe* r, const secp256k1_fe *n, const secp256k1_fe *d, const secp256k1_scalar *q, int known_on_curve) {

    secp256k1_fe g, i;
    secp256k1_ge p;
    secp256k1_gej rj;

    secp256k1_fe_sqr(&g, n);
    secp256k1_fe_mul(&g, &g, n);
    if (d) {
        secp256k1_fe b;
        VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero(d));
        secp256k1_fe_sqr(&b, d);
        VERIFY_CHECK(SECP256K1_B <= 8);
        secp256k1_fe_mul_int(&b, SECP256K1_B);
        secp256k1_fe_mul(&b, &b, d);
        secp256k1_fe_add(&g, &b);
        if (!known_on_curve) {

            secp256k1_fe c;
            secp256k1_fe_mul(&c, &g, d);
            if (!secp256k1_fe_is_square_var(&c)) return 0;
        }
    } else {
        secp256k1_fe_add_int(&g, SECP256K1_B);
        if (!known_on_curve) {

            if (!secp256k1_fe_is_square_var(&g)) return 0;
        }
    }

    secp256k1_fe_mul(&p.x, &g, n);
    secp256k1_fe_sqr(&p.y, &g);
    p.infinity = 0;

    VERIFY_CHECK(!secp256k1_scalar_is_zero(q));
    secp256k1_ecmult_const(&rj, &p, q);
    VERIFY_CHECK(!secp256k1_gej_is_infinity(&rj));

    secp256k1_fe_sqr(&i, &rj.z);
    secp256k1_fe_mul(&i, &i, &g);
    if (d) secp256k1_fe_mul(&i, &i, d);
    secp256k1_fe_inv(&i, &i);
    secp256k1_fe_mul(r, &rj.x, &i);

    return 1;
}

#endif
