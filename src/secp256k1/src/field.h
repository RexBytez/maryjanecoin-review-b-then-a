#ifndef SECP256K1_FIELD_H
#define SECP256K1_FIELD_H

#include "util.h"

#ifdef VERIFY
#  define SECP256K1_FE_VERIFY_FIELDS \
    int magnitude; \
    int normalized;
#else
#  define SECP256K1_FE_VERIFY_FIELDS
#endif

#if defined(SECP256K1_WIDEMUL_INT128)
#include "field_5x52.h"
#elif defined(SECP256K1_WIDEMUL_INT64)
#include "field_10x26.h"
#else
#error "Please select wide multiplication implementation"
#endif

#ifdef VERIFY

#define SECP256K1_FE_VERIFY_CONST(d7, d6, d5, d4, d3, d2, d1, d0) \
     \
    , (((d7) | (d6) | (d5) | (d4) | (d3) | (d2) | (d1) | (d0)) != 0) \
     \
    , (!(((d7) & (d6) & (d5) & (d4) & (d3) & (d2)) == 0xfffffffful && ((d1) == 0xfffffffful || ((d1) == 0xfffffffe && (d0 >= 0xfffffc2f)))))
#else
#define SECP256K1_FE_VERIFY_CONST(d7, d6, d5, d4, d3, d2, d1, d0)
#endif

#define SECP256K1_FE_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {SECP256K1_FE_CONST_INNER((d7), (d6), (d5), (d4), (d3), (d2), (d1), (d0)) SECP256K1_FE_VERIFY_CONST((d7), (d6), (d5), (d4), (d3), (d2), (d1), (d0)) }

static const secp256k1_fe secp256k1_fe_one = SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1);
static const secp256k1_fe secp256k1_const_beta = SECP256K1_FE_CONST(
    0x7ae96a2bul, 0x657c0710ul, 0x6e64479eul, 0xac3434e9ul,
    0x9cf04975ul, 0x12f58995ul, 0xc1396c28ul, 0x719501eeul
);

#ifndef VERIFY

#  define secp256k1_fe_normalize secp256k1_fe_impl_normalize
#  define secp256k1_fe_normalize_weak secp256k1_fe_impl_normalize_weak
#  define secp256k1_fe_normalize_var secp256k1_fe_impl_normalize_var
#  define secp256k1_fe_normalizes_to_zero secp256k1_fe_impl_normalizes_to_zero
#  define secp256k1_fe_normalizes_to_zero_var secp256k1_fe_impl_normalizes_to_zero_var
#  define secp256k1_fe_set_int secp256k1_fe_impl_set_int
#  define secp256k1_fe_clear secp256k1_fe_impl_clear
#  define secp256k1_fe_is_zero secp256k1_fe_impl_is_zero
#  define secp256k1_fe_is_odd secp256k1_fe_impl_is_odd
#  define secp256k1_fe_cmp_var secp256k1_fe_impl_cmp_var
#  define secp256k1_fe_set_b32_mod secp256k1_fe_impl_set_b32_mod
#  define secp256k1_fe_set_b32_limit secp256k1_fe_impl_set_b32_limit
#  define secp256k1_fe_get_b32 secp256k1_fe_impl_get_b32
#  define secp256k1_fe_negate_unchecked secp256k1_fe_impl_negate_unchecked
#  define secp256k1_fe_mul_int_unchecked secp256k1_fe_impl_mul_int_unchecked
#  define secp256k1_fe_add secp256k1_fe_impl_add
#  define secp256k1_fe_mul secp256k1_fe_impl_mul
#  define secp256k1_fe_sqr secp256k1_fe_impl_sqr
#  define secp256k1_fe_cmov secp256k1_fe_impl_cmov
#  define secp256k1_fe_to_storage secp256k1_fe_impl_to_storage
#  define secp256k1_fe_from_storage secp256k1_fe_impl_from_storage
#  define secp256k1_fe_inv secp256k1_fe_impl_inv
#  define secp256k1_fe_inv_var secp256k1_fe_impl_inv_var
#  define secp256k1_fe_get_bounds secp256k1_fe_impl_get_bounds
#  define secp256k1_fe_half secp256k1_fe_impl_half
#  define secp256k1_fe_add_int secp256k1_fe_impl_add_int
#  define secp256k1_fe_is_square_var secp256k1_fe_impl_is_square_var
#endif

static void secp256k1_fe_normalize(secp256k1_fe *r);

static void secp256k1_fe_normalize_weak(secp256k1_fe *r);

static void secp256k1_fe_normalize_var(secp256k1_fe *r);

static int secp256k1_fe_normalizes_to_zero(const secp256k1_fe *r);

static int secp256k1_fe_normalizes_to_zero_var(const secp256k1_fe *r);

static void secp256k1_fe_set_int(secp256k1_fe *r, int a);

static void secp256k1_fe_clear(secp256k1_fe *a);

static int secp256k1_fe_is_zero(const secp256k1_fe *a);

static int secp256k1_fe_is_odd(const secp256k1_fe *a);

static int secp256k1_fe_equal(const secp256k1_fe *a, const secp256k1_fe *b);

static int secp256k1_fe_cmp_var(const secp256k1_fe *a, const secp256k1_fe *b);

static void secp256k1_fe_set_b32_mod(secp256k1_fe *r, const unsigned char *a);

static int secp256k1_fe_set_b32_limit(secp256k1_fe *r, const unsigned char *a);

static void secp256k1_fe_get_b32(unsigned char *r, const secp256k1_fe *a);

#define secp256k1_fe_negate(r, a, m) ASSERT_INT_CONST_AND_DO(m, secp256k1_fe_negate_unchecked(r, a, m))

static void secp256k1_fe_negate_unchecked(secp256k1_fe *r, const secp256k1_fe *a, int m);

static void secp256k1_fe_add_int(secp256k1_fe *r, int a);

#define secp256k1_fe_mul_int(r, a) ASSERT_INT_CONST_AND_DO(a, secp256k1_fe_mul_int_unchecked(r, a))

static void secp256k1_fe_mul_int_unchecked(secp256k1_fe *r, int a);

static void secp256k1_fe_add(secp256k1_fe *r, const secp256k1_fe *a);

static void secp256k1_fe_mul(secp256k1_fe *r, const secp256k1_fe *a, const secp256k1_fe * SECP256K1_RESTRICT b);

static void secp256k1_fe_sqr(secp256k1_fe *r, const secp256k1_fe *a);

static int secp256k1_fe_sqrt(secp256k1_fe * SECP256K1_RESTRICT r, const secp256k1_fe * SECP256K1_RESTRICT a);

static void secp256k1_fe_inv(secp256k1_fe *r, const secp256k1_fe *a);

static void secp256k1_fe_inv_var(secp256k1_fe *r, const secp256k1_fe *a);

static void secp256k1_fe_to_storage(secp256k1_fe_storage *r, const secp256k1_fe *a);

static void secp256k1_fe_from_storage(secp256k1_fe *r, const secp256k1_fe_storage *a);

static void secp256k1_fe_storage_cmov(secp256k1_fe_storage *r, const secp256k1_fe_storage *a, int flag);

static void secp256k1_fe_cmov(secp256k1_fe *r, const secp256k1_fe *a, int flag);

static void secp256k1_fe_half(secp256k1_fe *r);

static void secp256k1_fe_get_bounds(secp256k1_fe *r, int m);

static int secp256k1_fe_is_square_var(const secp256k1_fe *a);

static void secp256k1_fe_verify(const secp256k1_fe *a);
#define SECP256K1_FE_VERIFY(a) secp256k1_fe_verify(a)

static void secp256k1_fe_verify_magnitude(const secp256k1_fe *a, int m);
#define SECP256K1_FE_VERIFY_MAGNITUDE(a, m) secp256k1_fe_verify_magnitude(a, m)

#endif
