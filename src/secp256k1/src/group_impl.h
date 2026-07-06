#ifndef SECP256K1_GROUP_IMPL_H
#define SECP256K1_GROUP_IMPL_H

#include "field.h"
#include "group.h"
#include "util.h"

#define SECP256K1_G_ORDER_7 SECP256K1_GE_CONST(\
    0x66625d13, 0x317ffe44, 0x63d32cff, 0x1ca02b9b,\
    0xe5c6d070, 0x50b4b05e, 0x81cc30db, 0xf5166f0a,\
    0x1e60e897, 0xa7c00c7c, 0x2df53eb6, 0x98274ff4,\
    0x64252f42, 0x8ca44e17, 0x3b25418c, 0xff4ab0cf\
)
#define SECP256K1_G_ORDER_13 SECP256K1_GE_CONST(\
    0xa2482ff8, 0x4bf34edf, 0xa51262fd, 0xe57921db,\
    0xe0dd2cb7, 0xa5914790, 0xbc71631f, 0xc09704fb,\
    0x942536cb, 0xa3e49492, 0x3a701cc3, 0xee3e443f,\
    0xdf182aa9, 0x15b8aa6a, 0x166d3b19, 0xba84b045\
)
#define SECP256K1_G_ORDER_199 SECP256K1_GE_CONST(\
    0x7fb07b5c, 0xd07c3bda, 0x553902e2, 0x7a87ea2c,\
    0x35108a7f, 0x051f41e5, 0xb76abad5, 0x1f2703ad,\
    0x0a251539, 0x5b4c4438, 0x952a634f, 0xac10dd4d,\
    0x6d6f4745, 0x98990c27, 0x3a4f3116, 0xd32ff969\
)

#define SECP256K1_G SECP256K1_GE_CONST(\
    0x79be667e, 0xf9dcbbac, 0x55a06295, 0xce870b07,\
    0x029bfcdb, 0x2dce28d9, 0x59f2815b, 0x16f81798,\
    0x483ada77, 0x26a3c465, 0x5da4fbfc, 0x0e1108a8,\
    0xfd17b448, 0xa6855419, 0x9c47d08f, 0xfb10d4b8\
)

#if defined(EXHAUSTIVE_TEST_ORDER)
#  if EXHAUSTIVE_TEST_ORDER == 7

static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_G_ORDER_7;
#define SECP256K1_B 6

#  elif EXHAUSTIVE_TEST_ORDER == 13

static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_G_ORDER_13;
#define SECP256K1_B 2

#  elif EXHAUSTIVE_TEST_ORDER == 199

static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_G_ORDER_199;
#define SECP256K1_B 4

#  else
#    error No known generator for the specified exhaustive test group order.
#  endif
#else

static const secp256k1_ge secp256k1_ge_const_g = SECP256K1_G;
#define SECP256K1_B 7

#endif

static void secp256k1_ge_verify(const secp256k1_ge *a) {
    SECP256K1_FE_VERIFY(&a->x);
    SECP256K1_FE_VERIFY(&a->y);
    SECP256K1_FE_VERIFY_MAGNITUDE(&a->x, SECP256K1_GE_X_MAGNITUDE_MAX);
    SECP256K1_FE_VERIFY_MAGNITUDE(&a->y, SECP256K1_GE_Y_MAGNITUDE_MAX);
    VERIFY_CHECK(a->infinity == 0 || a->infinity == 1);
    (void)a;
}

static void secp256k1_gej_verify(const secp256k1_gej *a) {
    SECP256K1_FE_VERIFY(&a->x);
    SECP256K1_FE_VERIFY(&a->y);
    SECP256K1_FE_VERIFY(&a->z);
    SECP256K1_FE_VERIFY_MAGNITUDE(&a->x, SECP256K1_GEJ_X_MAGNITUDE_MAX);
    SECP256K1_FE_VERIFY_MAGNITUDE(&a->y, SECP256K1_GEJ_Y_MAGNITUDE_MAX);
    SECP256K1_FE_VERIFY_MAGNITUDE(&a->z, SECP256K1_GEJ_Z_MAGNITUDE_MAX);
    VERIFY_CHECK(a->infinity == 0 || a->infinity == 1);
    (void)a;
}

static void secp256k1_ge_set_gej_zinv(secp256k1_ge *r, const secp256k1_gej *a, const secp256k1_fe *zi) {
    secp256k1_fe zi2;
    secp256k1_fe zi3;
    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_FE_VERIFY(zi);
    VERIFY_CHECK(!a->infinity);

    secp256k1_fe_sqr(&zi2, zi);
    secp256k1_fe_mul(&zi3, &zi2, zi);
    secp256k1_fe_mul(&r->x, &a->x, &zi2);
    secp256k1_fe_mul(&r->y, &a->y, &zi3);
    r->infinity = a->infinity;

    SECP256K1_GE_VERIFY(r);
}

static void secp256k1_ge_set_ge_zinv(secp256k1_ge *r, const secp256k1_ge *a, const secp256k1_fe *zi) {
    secp256k1_fe zi2;
    secp256k1_fe zi3;
    SECP256K1_GE_VERIFY(a);
    SECP256K1_FE_VERIFY(zi);
    VERIFY_CHECK(!a->infinity);

    secp256k1_fe_sqr(&zi2, zi);
    secp256k1_fe_mul(&zi3, &zi2, zi);
    secp256k1_fe_mul(&r->x, &a->x, &zi2);
    secp256k1_fe_mul(&r->y, &a->y, &zi3);
    r->infinity = a->infinity;

    SECP256K1_GE_VERIFY(r);
}

static void secp256k1_ge_set_xy(secp256k1_ge *r, const secp256k1_fe *x, const secp256k1_fe *y) {
    SECP256K1_FE_VERIFY(x);
    SECP256K1_FE_VERIFY(y);

    r->infinity = 0;
    r->x = *x;
    r->y = *y;

    SECP256K1_GE_VERIFY(r);
}

static int secp256k1_ge_is_infinity(const secp256k1_ge *a) {
    SECP256K1_GE_VERIFY(a);

    return a->infinity;
}

static void secp256k1_ge_neg(secp256k1_ge *r, const secp256k1_ge *a) {
    SECP256K1_GE_VERIFY(a);

    *r = *a;
    secp256k1_fe_normalize_weak(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);

    SECP256K1_GE_VERIFY(r);
}

static void secp256k1_ge_set_gej(secp256k1_ge *r, secp256k1_gej *a) {
    secp256k1_fe z2, z3;
    SECP256K1_GEJ_VERIFY(a);

    r->infinity = a->infinity;
    secp256k1_fe_inv(&a->z, &a->z);
    secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
    r->x = a->x;
    r->y = a->y;

    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_GE_VERIFY(r);
}

static void secp256k1_ge_set_gej_var(secp256k1_ge *r, secp256k1_gej *a) {
    secp256k1_fe z2, z3;
    SECP256K1_GEJ_VERIFY(a);

    if (secp256k1_gej_is_infinity(a)) {
        secp256k1_ge_set_infinity(r);
        return;
    }
    r->infinity = 0;
    secp256k1_fe_inv_var(&a->z, &a->z);
    secp256k1_fe_sqr(&z2, &a->z);
    secp256k1_fe_mul(&z3, &a->z, &z2);
    secp256k1_fe_mul(&a->x, &a->x, &z2);
    secp256k1_fe_mul(&a->y, &a->y, &z3);
    secp256k1_fe_set_int(&a->z, 1);
    secp256k1_ge_set_xy(r, &a->x, &a->y);

    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_GE_VERIFY(r);
}

static void secp256k1_ge_set_all_gej_var(secp256k1_ge *r, const secp256k1_gej *a, size_t len) {
    secp256k1_fe u;
    size_t i;
    size_t last_i = SIZE_MAX;
#ifdef VERIFY
    for (i = 0; i < len; i++) {
        SECP256K1_GEJ_VERIFY(&a[i]);
    }
#endif

    for (i = 0; i < len; i++) {
        if (a[i].infinity) {
            secp256k1_ge_set_infinity(&r[i]);
        } else {

            if (last_i == SIZE_MAX) {
                r[i].x = a[i].z;
            } else {
                secp256k1_fe_mul(&r[i].x, &r[last_i].x, &a[i].z);
            }
            last_i = i;
        }
    }
    if (last_i == SIZE_MAX) {
        return;
    }
    secp256k1_fe_inv_var(&u, &r[last_i].x);

    i = last_i;
    while (i > 0) {
        i--;
        if (!a[i].infinity) {
            secp256k1_fe_mul(&r[last_i].x, &r[i].x, &u);
            secp256k1_fe_mul(&u, &u, &a[last_i].z);
            last_i = i;
        }
    }
    VERIFY_CHECK(!a[last_i].infinity);
    r[last_i].x = u;

    for (i = 0; i < len; i++) {
        if (!a[i].infinity) {
            secp256k1_ge_set_gej_zinv(&r[i], &a[i], &r[i].x);
        }
    }

#ifdef VERIFY
    for (i = 0; i < len; i++) {
        SECP256K1_GE_VERIFY(&r[i]);
    }
#endif
}

static void secp256k1_ge_table_set_globalz(size_t len, secp256k1_ge *a, const secp256k1_fe *zr) {
    size_t i;
    secp256k1_fe zs;
#ifdef VERIFY
    for (i = 0; i < len; i++) {
        SECP256K1_GE_VERIFY(&a[i]);
        SECP256K1_FE_VERIFY(&zr[i]);
    }
#endif

    if (len > 0) {
        i = len - 1;

        secp256k1_fe_normalize_weak(&a[i].y);
        zs = zr[i];

        while (i > 0) {
            if (i != len - 1) {
                secp256k1_fe_mul(&zs, &zs, &zr[i]);
            }
            i--;
            secp256k1_ge_set_ge_zinv(&a[i], &a[i], &zs);
        }
    }

#ifdef VERIFY
    for (i = 0; i < len; i++) {
        SECP256K1_GE_VERIFY(&a[i]);
    }
#endif
}

static void secp256k1_gej_set_infinity(secp256k1_gej *r) {
    r->infinity = 1;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
    secp256k1_fe_clear(&r->z);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_ge_set_infinity(secp256k1_ge *r) {
    r->infinity = 1;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);

    SECP256K1_GE_VERIFY(r);
}

static void secp256k1_gej_clear(secp256k1_gej *r) {
    r->infinity = 0;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);
    secp256k1_fe_clear(&r->z);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_ge_clear(secp256k1_ge *r) {
    r->infinity = 0;
    secp256k1_fe_clear(&r->x);
    secp256k1_fe_clear(&r->y);

    SECP256K1_GE_VERIFY(r);
}

static int secp256k1_ge_set_xo_var(secp256k1_ge *r, const secp256k1_fe *x, int odd) {
    secp256k1_fe x2, x3;
    int ret;
    SECP256K1_FE_VERIFY(x);

    r->x = *x;
    secp256k1_fe_sqr(&x2, x);
    secp256k1_fe_mul(&x3, x, &x2);
    r->infinity = 0;
    secp256k1_fe_add_int(&x3, SECP256K1_B);
    ret = secp256k1_fe_sqrt(&r->y, &x3);
    secp256k1_fe_normalize_var(&r->y);
    if (secp256k1_fe_is_odd(&r->y) != odd) {
        secp256k1_fe_negate(&r->y, &r->y, 1);
    }

    SECP256K1_GE_VERIFY(r);
    return ret;
}

static void secp256k1_gej_set_ge(secp256k1_gej *r, const secp256k1_ge *a) {
   SECP256K1_GE_VERIFY(a);

   r->infinity = a->infinity;
   r->x = a->x;
   r->y = a->y;
   secp256k1_fe_set_int(&r->z, 1);

   SECP256K1_GEJ_VERIFY(r);
}

static int secp256k1_gej_eq_var(const secp256k1_gej *a, const secp256k1_gej *b) {
    secp256k1_gej tmp;
    SECP256K1_GEJ_VERIFY(b);
    SECP256K1_GEJ_VERIFY(a);

    secp256k1_gej_neg(&tmp, a);
    secp256k1_gej_add_var(&tmp, &tmp, b, NULL);
    return secp256k1_gej_is_infinity(&tmp);
}

static int secp256k1_gej_eq_ge_var(const secp256k1_gej *a, const secp256k1_ge *b) {
    secp256k1_gej tmp;
    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_GE_VERIFY(b);

    secp256k1_gej_neg(&tmp, a);
    secp256k1_gej_add_ge_var(&tmp, &tmp, b, NULL);
    return secp256k1_gej_is_infinity(&tmp);
}

static int secp256k1_ge_eq_var(const secp256k1_ge *a, const secp256k1_ge *b) {
    secp256k1_fe tmp;
    SECP256K1_GE_VERIFY(a);
    SECP256K1_GE_VERIFY(b);

    if (a->infinity != b->infinity) return 0;
    if (a->infinity) return 1;

    tmp = a->x;
    secp256k1_fe_normalize_weak(&tmp);
    if (!secp256k1_fe_equal(&tmp, &b->x)) return 0;

    tmp = a->y;
    secp256k1_fe_normalize_weak(&tmp);
    if (!secp256k1_fe_equal(&tmp, &b->y)) return 0;

    return 1;
}

static int secp256k1_gej_eq_x_var(const secp256k1_fe *x, const secp256k1_gej *a) {
    secp256k1_fe r;
    SECP256K1_FE_VERIFY(x);
    SECP256K1_GEJ_VERIFY(a);
    VERIFY_CHECK(!a->infinity);

    secp256k1_fe_sqr(&r, &a->z); secp256k1_fe_mul(&r, &r, x);
    return secp256k1_fe_equal(&r, &a->x);
}

static void secp256k1_gej_neg(secp256k1_gej *r, const secp256k1_gej *a) {
    SECP256K1_GEJ_VERIFY(a);

    r->infinity = a->infinity;
    r->x = a->x;
    r->y = a->y;
    r->z = a->z;
    secp256k1_fe_normalize_weak(&r->y);
    secp256k1_fe_negate(&r->y, &r->y, 1);

    SECP256K1_GEJ_VERIFY(r);
}

static int secp256k1_gej_is_infinity(const secp256k1_gej *a) {
    SECP256K1_GEJ_VERIFY(a);

    return a->infinity;
}

static int secp256k1_ge_is_valid_var(const secp256k1_ge *a) {
    secp256k1_fe y2, x3;
    SECP256K1_GE_VERIFY(a);

    if (a->infinity) {
        return 0;
    }

    secp256k1_fe_sqr(&y2, &a->y);
    secp256k1_fe_sqr(&x3, &a->x); secp256k1_fe_mul(&x3, &x3, &a->x);
    secp256k1_fe_add_int(&x3, SECP256K1_B);
    return secp256k1_fe_equal(&y2, &x3);
}

static SECP256K1_INLINE void secp256k1_gej_double(secp256k1_gej *r, const secp256k1_gej *a) {

    secp256k1_fe l, s, t;
    SECP256K1_GEJ_VERIFY(a);

    r->infinity = a->infinity;

    secp256k1_fe_mul(&r->z, &a->z, &a->y);
    secp256k1_fe_sqr(&s, &a->y);
    secp256k1_fe_sqr(&l, &a->x);
    secp256k1_fe_mul_int(&l, 3);
    secp256k1_fe_half(&l);
    secp256k1_fe_negate(&t, &s, 1);
    secp256k1_fe_mul(&t, &t, &a->x);
    secp256k1_fe_sqr(&r->x, &l);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_sqr(&s, &s);
    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &l);
    secp256k1_fe_add(&r->y, &s);
    secp256k1_fe_negate(&r->y, &r->y, 2);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_gej_double_var(secp256k1_gej *r, const secp256k1_gej *a, secp256k1_fe *rzr) {
    SECP256K1_GEJ_VERIFY(a);

    if (a->infinity) {
        secp256k1_gej_set_infinity(r);
        if (rzr != NULL) {
            secp256k1_fe_set_int(rzr, 1);
        }
        return;
    }

    if (rzr != NULL) {
        *rzr = a->y;
        secp256k1_fe_normalize_weak(rzr);
    }

    secp256k1_gej_double(r, a);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_gej_add_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_gej *b, secp256k1_fe *rzr) {

    secp256k1_fe z22, z12, u1, u2, s1, s2, h, i, h2, h3, t;
    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_GEJ_VERIFY(b);

    if (a->infinity) {
        VERIFY_CHECK(rzr == NULL);
        *r = *b;
        return;
    }
    if (b->infinity) {
        if (rzr != NULL) {
            secp256k1_fe_set_int(rzr, 1);
        }
        *r = *a;
        return;
    }

    secp256k1_fe_sqr(&z22, &b->z);
    secp256k1_fe_sqr(&z12, &a->z);
    secp256k1_fe_mul(&u1, &a->x, &z22);
    secp256k1_fe_mul(&u2, &b->x, &z12);
    secp256k1_fe_mul(&s1, &a->y, &z22); secp256k1_fe_mul(&s1, &s1, &b->z);
    secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &a->z);
    secp256k1_fe_negate(&h, &u1, 1); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_negate(&i, &s2, 1); secp256k1_fe_add(&i, &s1);
    if (secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (secp256k1_fe_normalizes_to_zero_var(&i)) {
            secp256k1_gej_double_var(r, a, rzr);
        } else {
            if (rzr != NULL) {
                secp256k1_fe_set_int(rzr, 0);
            }
            secp256k1_gej_set_infinity(r);
        }
        return;
    }

    r->infinity = 0;
    secp256k1_fe_mul(&t, &h, &b->z);
    if (rzr != NULL) {
        *rzr = t;
    }
    secp256k1_fe_mul(&r->z, &a->z, &t);

    secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_negate(&h2, &h2, 1);
    secp256k1_fe_mul(&h3, &h2, &h);
    secp256k1_fe_mul(&t, &u1, &h2);

    secp256k1_fe_sqr(&r->x, &i);
    secp256k1_fe_add(&r->x, &h3);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);

    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &i);
    secp256k1_fe_mul(&h3, &h3, &s1);
    secp256k1_fe_add(&r->y, &h3);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_gej_add_ge_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b, secp256k1_fe *rzr) {

    secp256k1_fe z12, u1, u2, s1, s2, h, i, h2, h3, t;
    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_GE_VERIFY(b);

    if (a->infinity) {
        VERIFY_CHECK(rzr == NULL);
        secp256k1_gej_set_ge(r, b);
        return;
    }
    if (b->infinity) {
        if (rzr != NULL) {
            secp256k1_fe_set_int(rzr, 1);
        }
        *r = *a;
        return;
    }

    secp256k1_fe_sqr(&z12, &a->z);
    u1 = a->x;
    secp256k1_fe_mul(&u2, &b->x, &z12);
    s1 = a->y;
    secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &a->z);
    secp256k1_fe_negate(&h, &u1, SECP256K1_GEJ_X_MAGNITUDE_MAX); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_negate(&i, &s2, 1); secp256k1_fe_add(&i, &s1);
    if (secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (secp256k1_fe_normalizes_to_zero_var(&i)) {
            secp256k1_gej_double_var(r, a, rzr);
        } else {
            if (rzr != NULL) {
                secp256k1_fe_set_int(rzr, 0);
            }
            secp256k1_gej_set_infinity(r);
        }
        return;
    }

    r->infinity = 0;
    if (rzr != NULL) {
        *rzr = h;
    }
    secp256k1_fe_mul(&r->z, &a->z, &h);

    secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_negate(&h2, &h2, 1);
    secp256k1_fe_mul(&h3, &h2, &h);
    secp256k1_fe_mul(&t, &u1, &h2);

    secp256k1_fe_sqr(&r->x, &i);
    secp256k1_fe_add(&r->x, &h3);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);

    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &i);
    secp256k1_fe_mul(&h3, &h3, &s1);
    secp256k1_fe_add(&r->y, &h3);

    SECP256K1_GEJ_VERIFY(r);
    if (rzr != NULL) SECP256K1_FE_VERIFY(rzr);
}

static void secp256k1_gej_add_zinv_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b, const secp256k1_fe *bzinv) {

    secp256k1_fe az, z12, u1, u2, s1, s2, h, i, h2, h3, t;
    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_GE_VERIFY(b);
    SECP256K1_FE_VERIFY(bzinv);

    if (a->infinity) {
        secp256k1_fe bzinv2, bzinv3;
        r->infinity = b->infinity;
        secp256k1_fe_sqr(&bzinv2, bzinv);
        secp256k1_fe_mul(&bzinv3, &bzinv2, bzinv);
        secp256k1_fe_mul(&r->x, &b->x, &bzinv2);
        secp256k1_fe_mul(&r->y, &b->y, &bzinv3);
        secp256k1_fe_set_int(&r->z, 1);
        SECP256K1_GEJ_VERIFY(r);
        return;
    }
    if (b->infinity) {
        *r = *a;
        return;
    }

    secp256k1_fe_mul(&az, &a->z, bzinv);

    secp256k1_fe_sqr(&z12, &az);
    u1 = a->x;
    secp256k1_fe_mul(&u2, &b->x, &z12);
    s1 = a->y;
    secp256k1_fe_mul(&s2, &b->y, &z12); secp256k1_fe_mul(&s2, &s2, &az);
    secp256k1_fe_negate(&h, &u1, SECP256K1_GEJ_X_MAGNITUDE_MAX); secp256k1_fe_add(&h, &u2);
    secp256k1_fe_negate(&i, &s2, 1); secp256k1_fe_add(&i, &s1);
    if (secp256k1_fe_normalizes_to_zero_var(&h)) {
        if (secp256k1_fe_normalizes_to_zero_var(&i)) {
            secp256k1_gej_double_var(r, a, NULL);
        } else {
            secp256k1_gej_set_infinity(r);
        }
        return;
    }

    r->infinity = 0;
    secp256k1_fe_mul(&r->z, &a->z, &h);

    secp256k1_fe_sqr(&h2, &h);
    secp256k1_fe_negate(&h2, &h2, 1);
    secp256k1_fe_mul(&h3, &h2, &h);
    secp256k1_fe_mul(&t, &u1, &h2);

    secp256k1_fe_sqr(&r->x, &i);
    secp256k1_fe_add(&r->x, &h3);
    secp256k1_fe_add(&r->x, &t);
    secp256k1_fe_add(&r->x, &t);

    secp256k1_fe_add(&t, &r->x);
    secp256k1_fe_mul(&r->y, &t, &i);
    secp256k1_fe_mul(&h3, &h3, &s1);
    secp256k1_fe_add(&r->y, &h3);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_gej_add_ge(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b) {

    secp256k1_fe zz, u1, u2, s1, s2, t, tt, m, n, q, rr;
    secp256k1_fe m_alt, rr_alt;
    int degenerate;
    SECP256K1_GEJ_VERIFY(a);
    SECP256K1_GE_VERIFY(b);
    VERIFY_CHECK(!b->infinity);

    secp256k1_fe_sqr(&zz, &a->z);
    u1 = a->x;
    secp256k1_fe_mul(&u2, &b->x, &zz);
    s1 = a->y;
    secp256k1_fe_mul(&s2, &b->y, &zz);
    secp256k1_fe_mul(&s2, &s2, &a->z);
    t = u1; secp256k1_fe_add(&t, &u2);
    m = s1; secp256k1_fe_add(&m, &s2);
    secp256k1_fe_sqr(&rr, &t);
    secp256k1_fe_negate(&m_alt, &u2, 1);
    secp256k1_fe_mul(&tt, &u1, &m_alt);
    secp256k1_fe_add(&rr, &tt);

    degenerate = secp256k1_fe_normalizes_to_zero(&m);

    rr_alt = s1;
    secp256k1_fe_mul_int(&rr_alt, 2);
    secp256k1_fe_add(&m_alt, &u1);

    secp256k1_fe_cmov(&rr_alt, &rr, !degenerate);
    secp256k1_fe_cmov(&m_alt, &m, !degenerate);

    secp256k1_fe_sqr(&n, &m_alt);
    secp256k1_fe_negate(&q, &t,
        SECP256K1_GEJ_X_MAGNITUDE_MAX + 1);
    secp256k1_fe_mul(&q, &q, &n);

    secp256k1_fe_sqr(&n, &n);
    secp256k1_fe_cmov(&n, &m, degenerate);
    secp256k1_fe_sqr(&t, &rr_alt);
    secp256k1_fe_mul(&r->z, &a->z, &m_alt);
    secp256k1_fe_add(&t, &q);
    r->x = t;
    secp256k1_fe_mul_int(&t, 2);
    secp256k1_fe_add(&t, &q);
    secp256k1_fe_mul(&t, &t, &rr_alt);
    secp256k1_fe_add(&t, &n);
    secp256k1_fe_negate(&r->y, &t,
        SECP256K1_GEJ_Y_MAGNITUDE_MAX + 2);
    secp256k1_fe_half(&r->y);

    secp256k1_fe_cmov(&r->x, &b->x, a->infinity);
    secp256k1_fe_cmov(&r->y, &b->y, a->infinity);
    secp256k1_fe_cmov(&r->z, &secp256k1_fe_one, a->infinity);

    r->infinity = secp256k1_fe_normalizes_to_zero(&r->z);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_gej_rescale(secp256k1_gej *r, const secp256k1_fe *s) {

    secp256k1_fe zz;
    SECP256K1_GEJ_VERIFY(r);
    SECP256K1_FE_VERIFY(s);
    VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero_var(s));

    secp256k1_fe_sqr(&zz, s);
    secp256k1_fe_mul(&r->x, &r->x, &zz);
    secp256k1_fe_mul(&r->y, &r->y, &zz);
    secp256k1_fe_mul(&r->y, &r->y, s);
    secp256k1_fe_mul(&r->z, &r->z, s);

    SECP256K1_GEJ_VERIFY(r);
}

static void secp256k1_ge_to_storage(secp256k1_ge_storage *r, const secp256k1_ge *a) {
    secp256k1_fe x, y;
    SECP256K1_GE_VERIFY(a);
    VERIFY_CHECK(!a->infinity);

    x = a->x;
    secp256k1_fe_normalize(&x);
    y = a->y;
    secp256k1_fe_normalize(&y);
    secp256k1_fe_to_storage(&r->x, &x);
    secp256k1_fe_to_storage(&r->y, &y);
}

static void secp256k1_ge_from_storage(secp256k1_ge *r, const secp256k1_ge_storage *a) {
    secp256k1_fe_from_storage(&r->x, &a->x);
    secp256k1_fe_from_storage(&r->y, &a->y);
    r->infinity = 0;

    SECP256K1_GE_VERIFY(r);
}

static SECP256K1_INLINE void secp256k1_gej_cmov(secp256k1_gej *r, const secp256k1_gej *a, int flag) {
    SECP256K1_GEJ_VERIFY(r);
    SECP256K1_GEJ_VERIFY(a);

    secp256k1_fe_cmov(&r->x, &a->x, flag);
    secp256k1_fe_cmov(&r->y, &a->y, flag);
    secp256k1_fe_cmov(&r->z, &a->z, flag);
    r->infinity ^= (r->infinity ^ a->infinity) & flag;

    SECP256K1_GEJ_VERIFY(r);
}

static SECP256K1_INLINE void secp256k1_ge_storage_cmov(secp256k1_ge_storage *r, const secp256k1_ge_storage *a, int flag) {
    secp256k1_fe_storage_cmov(&r->x, &a->x, flag);
    secp256k1_fe_storage_cmov(&r->y, &a->y, flag);
}

static void secp256k1_ge_mul_lambda(secp256k1_ge *r, const secp256k1_ge *a) {
    SECP256K1_GE_VERIFY(a);

    *r = *a;
    secp256k1_fe_mul(&r->x, &r->x, &secp256k1_const_beta);

    SECP256K1_GE_VERIFY(r);
}

static int secp256k1_ge_is_in_correct_subgroup(const secp256k1_ge* ge) {
#ifdef EXHAUSTIVE_TEST_ORDER
    secp256k1_gej out;
    int i;
    SECP256K1_GE_VERIFY(ge);

    secp256k1_gej_set_infinity(&out);
    for (i = 0; i < 32; ++i) {
        secp256k1_gej_double_var(&out, &out, NULL);
        if ((((uint32_t)EXHAUSTIVE_TEST_ORDER) >> (31 - i)) & 1) {
            secp256k1_gej_add_ge_var(&out, &out, ge, NULL);
        }
    }
    return secp256k1_gej_is_infinity(&out);
#else
    SECP256K1_GE_VERIFY(ge);

    (void)ge;

    return 1;
#endif
}

static int secp256k1_ge_x_on_curve_var(const secp256k1_fe *x) {
    secp256k1_fe c;
    secp256k1_fe_sqr(&c, x);
    secp256k1_fe_mul(&c, &c, x);
    secp256k1_fe_add_int(&c, SECP256K1_B);
    return secp256k1_fe_is_square_var(&c);
}

static int secp256k1_ge_x_frac_on_curve_var(const secp256k1_fe *xn, const secp256k1_fe *xd) {

     secp256k1_fe r, t;
     VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero_var(xd));

     secp256k1_fe_mul(&r, xd, xn);
     secp256k1_fe_sqr(&t, xn);
     secp256k1_fe_mul(&r, &r, &t);
     secp256k1_fe_sqr(&t, xd);
     secp256k1_fe_sqr(&t, &t);
     VERIFY_CHECK(SECP256K1_B <= 31);
     secp256k1_fe_mul_int(&t, SECP256K1_B);
     secp256k1_fe_add(&r, &t);
     return secp256k1_fe_is_square_var(&r);
}

#endif
