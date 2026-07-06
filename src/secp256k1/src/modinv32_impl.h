#ifndef SECP256K1_MODINV32_IMPL_H
#define SECP256K1_MODINV32_IMPL_H

#include "modinv32.h"

#include "util.h"

#include <stdlib.h>

#ifdef VERIFY
static const secp256k1_modinv32_signed30 SECP256K1_SIGNED30_ONE = {{1}};

static void secp256k1_modinv32_mul_30(secp256k1_modinv32_signed30 *r, const secp256k1_modinv32_signed30 *a, int alen, int32_t factor) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    int64_t c = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        if (i < alen) c += (int64_t)a->v[i] * factor;
        r->v[i] = (int32_t)c & M30; c >>= 30;
    }
    if (8 < alen) c += (int64_t)a->v[8] * factor;
    VERIFY_CHECK(c == (int32_t)c);
    r->v[8] = (int32_t)c;
}

static int secp256k1_modinv32_mul_cmp_30(const secp256k1_modinv32_signed30 *a, int alen, const secp256k1_modinv32_signed30 *b, int32_t factor) {
    int i;
    secp256k1_modinv32_signed30 am, bm;
    secp256k1_modinv32_mul_30(&am, a, alen, 1);
    secp256k1_modinv32_mul_30(&bm, b, 9, factor);
    for (i = 0; i < 8; ++i) {

        VERIFY_CHECK(am.v[i] >> 30 == 0);
        VERIFY_CHECK(bm.v[i] >> 30 == 0);
    }
    for (i = 8; i >= 0; --i) {
        if (am.v[i] < bm.v[i]) return -1;
        if (am.v[i] > bm.v[i]) return 1;
    }
    return 0;
}
#endif

static void secp256k1_modinv32_normalize_30(secp256k1_modinv32_signed30 *r, int32_t sign, const secp256k1_modinv32_modinfo *modinfo) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    int32_t r0 = r->v[0], r1 = r->v[1], r2 = r->v[2], r3 = r->v[3], r4 = r->v[4],
            r5 = r->v[5], r6 = r->v[6], r7 = r->v[7], r8 = r->v[8];
    volatile int32_t cond_add, cond_negate;

#ifdef VERIFY

    int i;
    for (i = 0; i < 9; ++i) {
        VERIFY_CHECK(r->v[i] >= -M30);
        VERIFY_CHECK(r->v[i] <= M30);
    }
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, 1) < 0);
#endif

    cond_add = r8 >> 31;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    r5 += modinfo->modulus.v[5] & cond_add;
    r6 += modinfo->modulus.v[6] & cond_add;
    r7 += modinfo->modulus.v[7] & cond_add;
    r8 += modinfo->modulus.v[8] & cond_add;
    cond_negate = sign >> 31;
    r0 = (r0 ^ cond_negate) - cond_negate;
    r1 = (r1 ^ cond_negate) - cond_negate;
    r2 = (r2 ^ cond_negate) - cond_negate;
    r3 = (r3 ^ cond_negate) - cond_negate;
    r4 = (r4 ^ cond_negate) - cond_negate;
    r5 = (r5 ^ cond_negate) - cond_negate;
    r6 = (r6 ^ cond_negate) - cond_negate;
    r7 = (r7 ^ cond_negate) - cond_negate;
    r8 = (r8 ^ cond_negate) - cond_negate;

    r1 += r0 >> 30; r0 &= M30;
    r2 += r1 >> 30; r1 &= M30;
    r3 += r2 >> 30; r2 &= M30;
    r4 += r3 >> 30; r3 &= M30;
    r5 += r4 >> 30; r4 &= M30;
    r6 += r5 >> 30; r5 &= M30;
    r7 += r6 >> 30; r6 &= M30;
    r8 += r7 >> 30; r7 &= M30;

    cond_add = r8 >> 31;
    r0 += modinfo->modulus.v[0] & cond_add;
    r1 += modinfo->modulus.v[1] & cond_add;
    r2 += modinfo->modulus.v[2] & cond_add;
    r3 += modinfo->modulus.v[3] & cond_add;
    r4 += modinfo->modulus.v[4] & cond_add;
    r5 += modinfo->modulus.v[5] & cond_add;
    r6 += modinfo->modulus.v[6] & cond_add;
    r7 += modinfo->modulus.v[7] & cond_add;
    r8 += modinfo->modulus.v[8] & cond_add;

    r1 += r0 >> 30; r0 &= M30;
    r2 += r1 >> 30; r1 &= M30;
    r3 += r2 >> 30; r2 &= M30;
    r4 += r3 >> 30; r3 &= M30;
    r5 += r4 >> 30; r4 &= M30;
    r6 += r5 >> 30; r5 &= M30;
    r7 += r6 >> 30; r6 &= M30;
    r8 += r7 >> 30; r7 &= M30;

    r->v[0] = r0;
    r->v[1] = r1;
    r->v[2] = r2;
    r->v[3] = r3;
    r->v[4] = r4;
    r->v[5] = r5;
    r->v[6] = r6;
    r->v[7] = r7;
    r->v[8] = r8;

    VERIFY_CHECK(r0 >> 30 == 0);
    VERIFY_CHECK(r1 >> 30 == 0);
    VERIFY_CHECK(r2 >> 30 == 0);
    VERIFY_CHECK(r3 >> 30 == 0);
    VERIFY_CHECK(r4 >> 30 == 0);
    VERIFY_CHECK(r5 >> 30 == 0);
    VERIFY_CHECK(r6 >> 30 == 0);
    VERIFY_CHECK(r7 >> 30 == 0);
    VERIFY_CHECK(r8 >> 30 == 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, 0) >= 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(r, 9, &modinfo->modulus, 1) < 0);
}

typedef struct {
    int32_t u, v, q, r;
} secp256k1_modinv32_trans2x2;

static int32_t secp256k1_modinv32_divsteps_30(int32_t zeta, uint32_t f0, uint32_t g0, secp256k1_modinv32_trans2x2 *t) {

    uint32_t u = 1, v = 0, q = 0, r = 1;
    volatile uint32_t c1, c2;
    uint32_t mask1, mask2, f = f0, g = g0, x, y, z;
    int i;

    for (i = 0; i < 30; ++i) {
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << i);
        VERIFY_CHECK((q * f0 + r * g0) == g << i);

        c1 = zeta >> 31;
        mask1 = c1;
        c2 = g & 1;
        mask2 = -c2;

        x = (f ^ mask1) - mask1;
        y = (u ^ mask1) - mask1;
        z = (v ^ mask1) - mask1;

        g += x & mask2;
        q += y & mask2;
        r += z & mask2;

        mask1 &= mask2;

        zeta = (zeta ^ mask1) - 1;

        f += g & mask1;
        u += q & mask1;
        v += r & mask1;

        g >>= 1;
        u <<= 1;
        v <<= 1;

        VERIFY_CHECK(zeta >= -601 && zeta <= 601);
    }

    t->u = (int32_t)u;
    t->v = (int32_t)v;
    t->q = (int32_t)q;
    t->r = (int32_t)r;

    VERIFY_CHECK((int64_t)t->u * t->r - (int64_t)t->v * t->q == ((int64_t)1) << 30);
    return zeta;
}

static const uint8_t secp256k1_modinv32_inv256[128] = {
    0xFF, 0x55, 0x33, 0x49, 0xC7, 0x5D, 0x3B, 0x11, 0x0F, 0xE5, 0xC3, 0x59,
    0xD7, 0xED, 0xCB, 0x21, 0x1F, 0x75, 0x53, 0x69, 0xE7, 0x7D, 0x5B, 0x31,
    0x2F, 0x05, 0xE3, 0x79, 0xF7, 0x0D, 0xEB, 0x41, 0x3F, 0x95, 0x73, 0x89,
    0x07, 0x9D, 0x7B, 0x51, 0x4F, 0x25, 0x03, 0x99, 0x17, 0x2D, 0x0B, 0x61,
    0x5F, 0xB5, 0x93, 0xA9, 0x27, 0xBD, 0x9B, 0x71, 0x6F, 0x45, 0x23, 0xB9,
    0x37, 0x4D, 0x2B, 0x81, 0x7F, 0xD5, 0xB3, 0xC9, 0x47, 0xDD, 0xBB, 0x91,
    0x8F, 0x65, 0x43, 0xD9, 0x57, 0x6D, 0x4B, 0xA1, 0x9F, 0xF5, 0xD3, 0xE9,
    0x67, 0xFD, 0xDB, 0xB1, 0xAF, 0x85, 0x63, 0xF9, 0x77, 0x8D, 0x6B, 0xC1,
    0xBF, 0x15, 0xF3, 0x09, 0x87, 0x1D, 0xFB, 0xD1, 0xCF, 0xA5, 0x83, 0x19,
    0x97, 0xAD, 0x8B, 0xE1, 0xDF, 0x35, 0x13, 0x29, 0xA7, 0x3D, 0x1B, 0xF1,
    0xEF, 0xC5, 0xA3, 0x39, 0xB7, 0xCD, 0xAB, 0x01
};

static int32_t secp256k1_modinv32_divsteps_30_var(int32_t eta, uint32_t f0, uint32_t g0, secp256k1_modinv32_trans2x2 *t) {

    uint32_t u = 1, v = 0, q = 0, r = 1;
    uint32_t f = f0, g = g0, m;
    uint16_t w;
    int i = 30, limit, zeros;

    for (;;) {

        zeros = secp256k1_ctz32_var(g | (UINT32_MAX << i));

        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;

        if (i == 0) break;
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((g & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << (30 - i));
        VERIFY_CHECK((q * f0 + r * g0) == g << (30 - i));

        VERIFY_CHECK(eta >= -751 && eta <= 751);

        if (eta < 0) {
            uint32_t tmp;
            eta = -eta;
            tmp = f; f = g; g = -tmp;
            tmp = u; u = q; q = -tmp;
            tmp = v; v = r; r = -tmp;
        }

        limit = ((int)eta + 1) > i ? i : ((int)eta + 1);

        VERIFY_CHECK(limit > 0 && limit <= 30);
        m = (UINT32_MAX >> (32 - limit)) & 255U;

        w = (g * secp256k1_modinv32_inv256[(f >> 1) & 127]) & m;

        g += f * w;
        q += u * w;
        r += v * w;
        VERIFY_CHECK((g & m) == 0);
    }

    t->u = (int32_t)u;
    t->v = (int32_t)v;
    t->q = (int32_t)q;
    t->r = (int32_t)r;

    VERIFY_CHECK((int64_t)t->u * t->r - (int64_t)t->v * t->q == ((int64_t)1) << 30);
    return eta;
}

static int32_t secp256k1_modinv32_posdivsteps_30_var(int32_t eta, uint32_t f0, uint32_t g0, secp256k1_modinv32_trans2x2 *t, int *jacp) {

    uint32_t u = 1, v = 0, q = 0, r = 1;
    uint32_t f = f0, g = g0, m;
    uint16_t w;
    int i = 30, limit, zeros;
    int jac = *jacp;

    for (;;) {

        zeros = secp256k1_ctz32_var(g | (UINT32_MAX << i));

        g >>= zeros;
        u <<= zeros;
        v <<= zeros;
        eta -= zeros;
        i -= zeros;

        jac ^= (zeros & ((f >> 1) ^ (f >> 2)));

        if (i == 0) break;
        VERIFY_CHECK((f & 1) == 1);
        VERIFY_CHECK((g & 1) == 1);
        VERIFY_CHECK((u * f0 + v * g0) == f << (30 - i));
        VERIFY_CHECK((q * f0 + r * g0) == g << (30 - i));

        if (eta < 0) {
            uint32_t tmp;
            eta = -eta;

            jac ^= ((f & g) >> 1);
            tmp = f; f = g; g = tmp;
            tmp = u; u = q; q = tmp;
            tmp = v; v = r; r = tmp;
        }

        limit = ((int)eta + 1) > i ? i : ((int)eta + 1);

        VERIFY_CHECK(limit > 0 && limit <= 30);
        m = (UINT32_MAX >> (32 - limit)) & 255U;

        w = (g * secp256k1_modinv32_inv256[(f >> 1) & 127]) & m;

        g += f * w;
        q += u * w;
        r += v * w;
        VERIFY_CHECK((g & m) == 0);
    }

    t->u = (int32_t)u;
    t->v = (int32_t)v;
    t->q = (int32_t)q;
    t->r = (int32_t)r;

    VERIFY_CHECK((int64_t)t->u * t->r - (int64_t)t->v * t->q == ((int64_t)1) << 30 ||
                 (int64_t)t->u * t->r - (int64_t)t->v * t->q == -(((int64_t)1) << 30));
    *jacp = jac;
    return eta;
}

static void secp256k1_modinv32_update_de_30(secp256k1_modinv32_signed30 *d, secp256k1_modinv32_signed30 *e, const secp256k1_modinv32_trans2x2 *t, const secp256k1_modinv32_modinfo* modinfo) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    const int32_t u = t->u, v = t->v, q = t->q, r = t->r;
    int32_t di, ei, md, me, sd, se;
    int64_t cd, ce;
    int i;
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, 1) < 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, 1) < 0);
    VERIFY_CHECK(labs(u) <= (M30 + 1 - labs(v)));
    VERIFY_CHECK(labs(q) <= (M30 + 1 - labs(r)));

    sd = d->v[8] >> 31;
    se = e->v[8] >> 31;
    md = (u & sd) + (v & se);
    me = (q & sd) + (r & se);

    di = d->v[0];
    ei = e->v[0];
    cd = (int64_t)u * di + (int64_t)v * ei;
    ce = (int64_t)q * di + (int64_t)r * ei;

    md -= (modinfo->modulus_inv30 * (uint32_t)cd + md) & M30;
    me -= (modinfo->modulus_inv30 * (uint32_t)ce + me) & M30;

    cd += (int64_t)modinfo->modulus.v[0] * md;
    ce += (int64_t)modinfo->modulus.v[0] * me;

    VERIFY_CHECK(((int32_t)cd & M30) == 0); cd >>= 30;
    VERIFY_CHECK(((int32_t)ce & M30) == 0); ce >>= 30;

    for (i = 1; i < 9; ++i) {
        di = d->v[i];
        ei = e->v[i];
        cd += (int64_t)u * di + (int64_t)v * ei;
        ce += (int64_t)q * di + (int64_t)r * ei;
        cd += (int64_t)modinfo->modulus.v[i] * md;
        ce += (int64_t)modinfo->modulus.v[i] * me;
        d->v[i - 1] = (int32_t)cd & M30; cd >>= 30;
        e->v[i - 1] = (int32_t)ce & M30; ce >>= 30;
    }

    d->v[8] = (int32_t)cd;
    e->v[8] = (int32_t)ce;

    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(d, 9, &modinfo->modulus, 1) < 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, -2) > 0);
    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(e, 9, &modinfo->modulus, 1) < 0);
}

static void secp256k1_modinv32_update_fg_30(secp256k1_modinv32_signed30 *f, secp256k1_modinv32_signed30 *g, const secp256k1_modinv32_trans2x2 *t) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    const int32_t u = t->u, v = t->v, q = t->q, r = t->r;
    int32_t fi, gi;
    int64_t cf, cg;
    int i;

    fi = f->v[0];
    gi = g->v[0];
    cf = (int64_t)u * fi + (int64_t)v * gi;
    cg = (int64_t)q * fi + (int64_t)r * gi;

    VERIFY_CHECK(((int32_t)cf & M30) == 0); cf >>= 30;
    VERIFY_CHECK(((int32_t)cg & M30) == 0); cg >>= 30;

    for (i = 1; i < 9; ++i) {
        fi = f->v[i];
        gi = g->v[i];
        cf += (int64_t)u * fi + (int64_t)v * gi;
        cg += (int64_t)q * fi + (int64_t)r * gi;
        f->v[i - 1] = (int32_t)cf & M30; cf >>= 30;
        g->v[i - 1] = (int32_t)cg & M30; cg >>= 30;
    }

    f->v[8] = (int32_t)cf;
    g->v[8] = (int32_t)cg;
}

static void secp256k1_modinv32_update_fg_30_var(int len, secp256k1_modinv32_signed30 *f, secp256k1_modinv32_signed30 *g, const secp256k1_modinv32_trans2x2 *t) {
    const int32_t M30 = (int32_t)(UINT32_MAX >> 2);
    const int32_t u = t->u, v = t->v, q = t->q, r = t->r;
    int32_t fi, gi;
    int64_t cf, cg;
    int i;
    VERIFY_CHECK(len > 0);

    fi = f->v[0];
    gi = g->v[0];
    cf = (int64_t)u * fi + (int64_t)v * gi;
    cg = (int64_t)q * fi + (int64_t)r * gi;

    VERIFY_CHECK(((int32_t)cf & M30) == 0); cf >>= 30;
    VERIFY_CHECK(((int32_t)cg & M30) == 0); cg >>= 30;

    for (i = 1; i < len; ++i) {
        fi = f->v[i];
        gi = g->v[i];
        cf += (int64_t)u * fi + (int64_t)v * gi;
        cg += (int64_t)q * fi + (int64_t)r * gi;
        f->v[i - 1] = (int32_t)cf & M30; cf >>= 30;
        g->v[i - 1] = (int32_t)cg & M30; cg >>= 30;
    }

    f->v[len - 1] = (int32_t)cf;
    g->v[len - 1] = (int32_t)cg;
}

static void secp256k1_modinv32(secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo) {

    secp256k1_modinv32_signed30 d = {{0}};
    secp256k1_modinv32_signed30 e = {{1}};
    secp256k1_modinv32_signed30 f = modinfo->modulus;
    secp256k1_modinv32_signed30 g = *x;
    int i;
    int32_t zeta = -1;

    for (i = 0; i < 20; ++i) {

        secp256k1_modinv32_trans2x2 t;
        zeta = secp256k1_modinv32_divsteps_30(zeta, f.v[0], g.v[0], &t);

        secp256k1_modinv32_update_de_30(&d, &e, &t, modinfo);

        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, 1) < 0);

        secp256k1_modinv32_update_fg_30(&f, &g, &t);

        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &modinfo->modulus, 1) < 0);
    }

    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, 9, &SECP256K1_SIGNED30_ONE, 0) == 0);

    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, 9, &SECP256K1_SIGNED30_ONE, -1) == 0 ||
                 secp256k1_modinv32_mul_cmp_30(&f, 9, &SECP256K1_SIGNED30_ONE, 1) == 0 ||
                 (secp256k1_modinv32_mul_cmp_30(x, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  secp256k1_modinv32_mul_cmp_30(&d, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  (secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv32_mul_cmp_30(&f, 9, &modinfo->modulus, -1) == 0)));

    secp256k1_modinv32_normalize_30(&d, f.v[8], modinfo);
    *x = d;
}

static void secp256k1_modinv32_var(secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo) {

    secp256k1_modinv32_signed30 d = {{0, 0, 0, 0, 0, 0, 0, 0, 0}};
    secp256k1_modinv32_signed30 e = {{1, 0, 0, 0, 0, 0, 0, 0, 0}};
    secp256k1_modinv32_signed30 f = modinfo->modulus;
    secp256k1_modinv32_signed30 g = *x;
#ifdef VERIFY
    int i = 0;
#endif
    int j, len = 9;
    int32_t eta = -1;
    int32_t cond, fn, gn;

    while (1) {

        secp256k1_modinv32_trans2x2 t;
        eta = secp256k1_modinv32_divsteps_30_var(eta, f.v[0], g.v[0], &t);

        secp256k1_modinv32_update_de_30(&d, &e, &t, modinfo);

        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);

        secp256k1_modinv32_update_fg_30_var(len, &f, &g, &t);

        if (g.v[0] == 0) {
            cond = 0;

            for (j = 1; j < len; ++j) {
                cond |= g.v[j];
            }

            if (cond == 0) break;
        }

        fn = f.v[len - 1];
        gn = g.v[len - 1];
        cond = ((int32_t)len - 2) >> 31;
        cond |= fn ^ (fn >> 31);
        cond |= gn ^ (gn >> 31);

        if (cond == 0) {
            f.v[len - 2] |= (uint32_t)fn << 30;
            g.v[len - 2] |= (uint32_t)gn << 30;
            --len;
        }

        VERIFY_CHECK(++i < 25);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, -1) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);
    }

    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &SECP256K1_SIGNED30_ONE, 0) == 0);

    VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &SECP256K1_SIGNED30_ONE, -1) == 0 ||
                 secp256k1_modinv32_mul_cmp_30(&f, len, &SECP256K1_SIGNED30_ONE, 1) == 0 ||
                 (secp256k1_modinv32_mul_cmp_30(x, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  secp256k1_modinv32_mul_cmp_30(&d, 9, &SECP256K1_SIGNED30_ONE, 0) == 0 &&
                  (secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) == 0 ||
                   secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, -1) == 0)));

    secp256k1_modinv32_normalize_30(&d, f.v[len - 1], modinfo);
    *x = d;
}

#ifdef VERIFY
#define JACOBI32_ITERATIONS 25
#else
#define JACOBI32_ITERATIONS 50
#endif

static int secp256k1_jacobi32_maybe_var(const secp256k1_modinv32_signed30 *x, const secp256k1_modinv32_modinfo *modinfo) {

    secp256k1_modinv32_signed30 f = modinfo->modulus;
    secp256k1_modinv32_signed30 g = *x;
    int j, len = 9;
    int32_t eta = -1;
    int32_t cond, fn, gn;
    int jac = 0;
    int count;

    VERIFY_CHECK(g.v[0] >= 0 && g.v[1] >= 0 && g.v[2] >= 0 && g.v[3] >= 0 && g.v[4] >= 0 && g.v[5] >= 0 && g.v[6] >= 0 && g.v[7] >= 0 && g.v[8] >= 0);

    VERIFY_CHECK((g.v[0] | g.v[1] | g.v[2] | g.v[3] | g.v[4] | g.v[5] | g.v[6] | g.v[7] | g.v[8]) != 0);

    for (count = 0; count < JACOBI32_ITERATIONS; ++count) {

        secp256k1_modinv32_trans2x2 t;
        eta = secp256k1_modinv32_posdivsteps_30_var(eta, f.v[0] | ((uint32_t)f.v[1] << 30), g.v[0] | ((uint32_t)g.v[1] << 30), &t, &jac);

        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 0) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 0) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);

        secp256k1_modinv32_update_fg_30_var(len, &f, &g, &t);

        if (f.v[0] == 1) {
            cond = 0;

            for (j = 1; j < len; ++j) {
                cond |= f.v[j];
            }

            if (cond == 0) return 1 - 2*(jac & 1);
        }

        fn = f.v[len - 1];
        gn = g.v[len - 1];
        cond = ((int32_t)len - 2) >> 31;
        cond |= fn;
        cond |= gn;

        if (cond == 0) --len;

        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 0) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&f, len, &modinfo->modulus, 1) <= 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 0) > 0);
        VERIFY_CHECK(secp256k1_modinv32_mul_cmp_30(&g, len, &modinfo->modulus, 1) < 0);
    }

    return 0;
}

#endif
