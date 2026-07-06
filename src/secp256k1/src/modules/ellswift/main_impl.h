#ifndef SECP256K1_MODULE_ELLSWIFT_MAIN_H
#define SECP256K1_MODULE_ELLSWIFT_MAIN_H

#include "../../../include/secp256k1.h"
#include "../../../include/secp256k1_ellswift.h"
#include "../../eckey.h"
#include "../../hash.h"

static const secp256k1_fe secp256k1_ellswift_c1 = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa40);

static const secp256k1_fe secp256k1_ellswift_c2 = SECP256K1_FE_CONST(0x7ae96a2b, 0x657c0710, 0x6e64479e, 0xac3434e9, 0x9cf04975, 0x12f58995, 0xc1396c28, 0x719501ee);

static const secp256k1_fe secp256k1_ellswift_c3 = SECP256K1_FE_CONST(0x7ae96a2b, 0x657c0710, 0x6e64479e, 0xac3434e9, 0x9cf04975, 0x12f58995, 0xc1396c28, 0x719501ef);

static const secp256k1_fe secp256k1_ellswift_c4 = SECP256K1_FE_CONST(0x851695d4, 0x9a83f8ef, 0x919bb861, 0x53cbcb16, 0x630fb68a, 0xed0a766a, 0x3ec693d6, 0x8e6afa41);

static void secp256k1_ellswift_xswiftec_frac_var(secp256k1_fe *xn, secp256k1_fe *xd, const secp256k1_fe *u, const secp256k1_fe *t) {

    secp256k1_fe u1, s, g, p, d, n, l;
    u1 = *u;
    if (EXPECT(secp256k1_fe_normalizes_to_zero_var(&u1), 0)) u1 = secp256k1_fe_one;
    secp256k1_fe_sqr(&s, t);
    if (EXPECT(secp256k1_fe_normalizes_to_zero_var(t), 0)) s = secp256k1_fe_one;
    secp256k1_fe_sqr(&l, &u1);
    secp256k1_fe_mul(&g, &l, &u1);
    secp256k1_fe_add_int(&g, SECP256K1_B);
    p = g;
    secp256k1_fe_add(&p, &s);
    if (EXPECT(secp256k1_fe_normalizes_to_zero_var(&p), 0)) {
        secp256k1_fe_mul_int(&s, 4);

        p = g;
        secp256k1_fe_add(&p, &s);
    }
    secp256k1_fe_mul(&d, &s, &l);
    secp256k1_fe_mul_int(&d, 3);
    secp256k1_fe_sqr(&l, &p);
    secp256k1_fe_negate(&l, &l, 1);
    secp256k1_fe_mul(&n, &d, &u1);
    secp256k1_fe_add(&n, &l);
    if (secp256k1_ge_x_frac_on_curve_var(&n, &d)) {

        *xn = n;
        *xd = d;
        return;
    }
    *xd = p;
    secp256k1_fe_mul(&l, &secp256k1_ellswift_c1, &s);
    secp256k1_fe_mul(&n, &secp256k1_ellswift_c2, &g);
    secp256k1_fe_add(&n, &l);
    secp256k1_fe_mul(&n, &n, &u1);

    if (secp256k1_ge_x_frac_on_curve_var(&n, &p)) {

        *xn = n;
        return;
    }
    secp256k1_fe_mul(&l, &p, &u1);
    secp256k1_fe_add(&n, &l);
    secp256k1_fe_negate(xn, &n, 2);

    VERIFY_CHECK(secp256k1_ge_x_frac_on_curve_var(xn, &p));

}

static void secp256k1_ellswift_xswiftec_var(secp256k1_fe *x, const secp256k1_fe *u, const secp256k1_fe *t) {
    secp256k1_fe xn, xd;
    secp256k1_ellswift_xswiftec_frac_var(&xn, &xd, u, t);
    secp256k1_fe_inv_var(&xd, &xd);
    secp256k1_fe_mul(x, &xn, &xd);
}

static void secp256k1_ellswift_swiftec_var(secp256k1_ge *p, const secp256k1_fe *u, const secp256k1_fe *t) {
    secp256k1_fe x;
    secp256k1_ellswift_xswiftec_var(&x, u, t);
    secp256k1_ge_set_xo_var(p, &x, secp256k1_fe_is_odd(t));
}

static int secp256k1_ellswift_xswiftec_inv_var(secp256k1_fe *t, const secp256k1_fe *x_in, const secp256k1_fe *u_in, int c) {

    secp256k1_fe x = *x_in, u = *u_in, g, v, s, m, r, q;
    int ret;

    secp256k1_fe_normalize_weak(&x);
    secp256k1_fe_normalize_weak(&u);

    VERIFY_CHECK(c >= 0 && c < 8);
    VERIFY_CHECK(secp256k1_ge_x_on_curve_var(&x));

    if (!(c & 2)) {

        m = x;
        secp256k1_fe_add(&m, &u);
        secp256k1_fe_negate(&m, &m, 2);

        if (secp256k1_ge_x_on_curve_var(&m)) return 0;

        secp256k1_fe_sqr(&s, &m);
        secp256k1_fe_negate(&s, &s, 1);
        secp256k1_fe_mul(&m, &u, &x);
        secp256k1_fe_add(&s, &m);

        VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero_var(&s));

        secp256k1_fe_sqr(&g, &u);
        secp256k1_fe_mul(&g, &g, &u);
        secp256k1_fe_add_int(&g, SECP256K1_B);
        secp256k1_fe_mul(&m, &s, &g);
        if (!secp256k1_fe_is_square_var(&m)) return 0;

        secp256k1_fe_inv_var(&s, &s);
        secp256k1_fe_mul(&s, &s, &g);

        v = x;
    } else {

        secp256k1_fe_negate(&m, &u, 1);
        s = m;
        secp256k1_fe_add(&s, &x);

        if (!secp256k1_fe_is_square_var(&s)) return 0;

        secp256k1_fe_sqr(&g, &u);
        secp256k1_fe_mul(&q, &s, &g);
        secp256k1_fe_mul_int(&q, 3);
        secp256k1_fe_mul(&g, &g, &u);
        secp256k1_fe_mul_int(&g, 4);
        secp256k1_fe_add_int(&g, 4 * SECP256K1_B);
        secp256k1_fe_add(&q, &g);
        secp256k1_fe_mul(&q, &q, &s);
        secp256k1_fe_negate(&q, &q, 1);
        if (!secp256k1_fe_is_square_var(&q)) return 0;
        ret = secp256k1_fe_sqrt(&r, &q);
#ifdef VERIFY
        VERIFY_CHECK(ret);
#else
        (void)ret;
#endif

        if (EXPECT((c & 1) && secp256k1_fe_normalizes_to_zero_var(&r), 0)) return 0;

        if (EXPECT(secp256k1_fe_normalizes_to_zero_var(&s), 0)) return 0;

        secp256k1_fe_inv_var(&v, &s);
        secp256k1_fe_mul(&v, &v, &r);
        secp256k1_fe_add(&v, &m);
        secp256k1_fe_half(&v);
    }

    ret = secp256k1_fe_sqrt(&m, &s);
    VERIFY_CHECK(ret);

    if ((c & 5) == 0 || (c & 5) == 5) {
        secp256k1_fe_negate(&m, &m, 1);
    }

    secp256k1_fe_mul(&u, &u, c&1 ? &secp256k1_ellswift_c4 : &secp256k1_ellswift_c3);

    secp256k1_fe_add(&u, &v);
    secp256k1_fe_mul(t, &m, &u);
    return 1;
}

static void secp256k1_ellswift_prng(unsigned char* out32, const secp256k1_sha256 *hasher, uint32_t cnt) {
    secp256k1_sha256 hash = *hasher;
    unsigned char buf4[4];
#ifdef VERIFY
    size_t blocks = hash.bytes >> 6;
#endif
    buf4[0] = cnt;
    buf4[1] = cnt >> 8;
    buf4[2] = cnt >> 16;
    buf4[3] = cnt >> 24;
    secp256k1_sha256_write(&hash, buf4, 4);
    secp256k1_sha256_finalize(&hash, out32);

    VERIFY_CHECK(((hash.bytes) >> 6) == (blocks + 1));
}

static void secp256k1_ellswift_xelligatorswift_var(unsigned char *u32, secp256k1_fe *t, const secp256k1_fe *x, const secp256k1_sha256 *hasher) {

    unsigned char branch_hash[32];

    int branches_left = 0;

    uint32_t cnt = 0;
    while (1) {
        int branch;
        secp256k1_fe u;

        if (branches_left == 0) {
            secp256k1_ellswift_prng(branch_hash, hasher, cnt++);
            branches_left = 64;
        }

        --branches_left;
        branch = (branch_hash[branches_left >> 1] >> ((branches_left & 1) << 2)) & 7;

        secp256k1_ellswift_prng(u32, hasher, cnt++);

        secp256k1_fe_set_b32_mod(&u, u32);

        VERIFY_CHECK(!secp256k1_fe_normalizes_to_zero_var(&u));

        if (EXPECT(secp256k1_ellswift_xswiftec_inv_var(t, x, &u, branch), 0)) break;
    }
}

static void secp256k1_ellswift_elligatorswift_var(unsigned char *u32, secp256k1_fe *t, const secp256k1_ge *p, const secp256k1_sha256 *hasher) {
    secp256k1_ellswift_xelligatorswift_var(u32, t, &p->x, hasher);
    secp256k1_fe_normalize_var(t);
    if (secp256k1_fe_is_odd(t) != secp256k1_fe_is_odd(&p->y)) {
        secp256k1_fe_negate(t, t, 1);
        secp256k1_fe_normalize_var(t);
    }
}

static void secp256k1_ellswift_sha256_init_encode(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0xd1a6524bul;
    hash->s[1] = 0x028594b3ul;
    hash->s[2] = 0x96e42f4eul;
    hash->s[3] = 0x1037a177ul;
    hash->s[4] = 0x1b8fcb8bul;
    hash->s[5] = 0x56023885ul;
    hash->s[6] = 0x2560ede1ul;
    hash->s[7] = 0xd626b715ul;

    hash->bytes = 64;
}

int secp256k1_ellswift_encode(const secp256k1_context *ctx, unsigned char *ell64, const secp256k1_pubkey *pubkey, const unsigned char *rnd32) {
    secp256k1_ge p;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(ell64 != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(rnd32 != NULL);

    if (secp256k1_pubkey_load(ctx, &p, pubkey)) {
        secp256k1_fe t;
        unsigned char p64[64] = {0};
        size_t ser_size;
        int ser_ret;
        secp256k1_sha256 hash;

        secp256k1_ellswift_sha256_init_encode(&hash);
        ser_ret = secp256k1_eckey_pubkey_serialize(&p, p64, &ser_size, 1);
#ifdef VERIFY
        VERIFY_CHECK(ser_ret && ser_size == 33);
#else
        (void)ser_ret;
#endif
        secp256k1_sha256_write(&hash, p64, sizeof(p64));
        secp256k1_sha256_write(&hash, rnd32, 32);

        secp256k1_ellswift_elligatorswift_var(ell64, &t, &p, &hash);
        secp256k1_fe_get_b32(ell64 + 32, &t);
        return 1;
    }

    memset(ell64, 0, 64);
    return 0;
}

static void secp256k1_ellswift_sha256_init_create(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0xd29e1bf5ul;
    hash->s[1] = 0xf7025f42ul;
    hash->s[2] = 0x9b024773ul;
    hash->s[3] = 0x094cb7d5ul;
    hash->s[4] = 0xe59ed789ul;
    hash->s[5] = 0x03bc9786ul;
    hash->s[6] = 0x68335b35ul;
    hash->s[7] = 0x4e363b53ul;

    hash->bytes = 64;
}

int secp256k1_ellswift_create(const secp256k1_context *ctx, unsigned char *ell64, const unsigned char *seckey32, const unsigned char *auxrnd32) {
    secp256k1_ge p;
    secp256k1_fe t;
    secp256k1_sha256 hash;
    secp256k1_scalar seckey_scalar;
    int ret;
    static const unsigned char zero32[32] = {0};

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(ell64 != NULL);
    memset(ell64, 0, 64);
    ARG_CHECK(secp256k1_ecmult_gen_context_is_built(&ctx->ecmult_gen_ctx));
    ARG_CHECK(seckey32 != NULL);

    ret = secp256k1_ec_pubkey_create_helper(&ctx->ecmult_gen_ctx, &seckey_scalar, &p, seckey32);
    secp256k1_declassify(ctx, &p, sizeof(p));
    secp256k1_fe_normalize_var(&p.x);
    secp256k1_fe_normalize_var(&p.y);

    secp256k1_ellswift_sha256_init_create(&hash);
    secp256k1_sha256_write(&hash, seckey32, 32);
    secp256k1_sha256_write(&hash, zero32, sizeof(zero32));
    secp256k1_declassify(ctx, &hash, sizeof(hash));
    if (auxrnd32) secp256k1_sha256_write(&hash, auxrnd32, 32);

    secp256k1_ellswift_elligatorswift_var(ell64, &t, &p, &hash);
    secp256k1_fe_get_b32(ell64 + 32, &t);

    secp256k1_memczero(ell64, 64, !ret);
    secp256k1_scalar_clear(&seckey_scalar);

    return ret;
}

int secp256k1_ellswift_decode(const secp256k1_context *ctx, secp256k1_pubkey *pubkey, const unsigned char *ell64) {
    secp256k1_fe u, t;
    secp256k1_ge p;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(pubkey != NULL);
    ARG_CHECK(ell64 != NULL);

    secp256k1_fe_set_b32_mod(&u, ell64);
    secp256k1_fe_set_b32_mod(&t, ell64 + 32);
    secp256k1_fe_normalize_var(&t);
    secp256k1_ellswift_swiftec_var(&p, &u, &t);
    secp256k1_pubkey_save(pubkey, &p);
    return 1;
}

static int ellswift_xdh_hash_function_prefix(unsigned char *output, const unsigned char *x32, const unsigned char *ell_a64, const unsigned char *ell_b64, void *data) {
    secp256k1_sha256 sha;

    secp256k1_sha256_initialize(&sha);
    secp256k1_sha256_write(&sha, data, 64);
    secp256k1_sha256_write(&sha, ell_a64, 64);
    secp256k1_sha256_write(&sha, ell_b64, 64);
    secp256k1_sha256_write(&sha, x32, 32);
    secp256k1_sha256_finalize(&sha, output);

    return 1;
}

static void secp256k1_ellswift_sha256_init_bip324(secp256k1_sha256* hash) {
    secp256k1_sha256_initialize(hash);
    hash->s[0] = 0x8c12d730ul;
    hash->s[1] = 0x827bd392ul;
    hash->s[2] = 0x9e4fb2eeul;
    hash->s[3] = 0x207b373eul;
    hash->s[4] = 0x2292bd7aul;
    hash->s[5] = 0xaa5441bcul;
    hash->s[6] = 0x15c3779ful;
    hash->s[7] = 0xcfb52549ul;

    hash->bytes = 64;
}

static int ellswift_xdh_hash_function_bip324(unsigned char* output, const unsigned char *x32, const unsigned char *ell_a64, const unsigned char *ell_b64, void *data) {
    secp256k1_sha256 sha;

    (void)data;

    secp256k1_ellswift_sha256_init_bip324(&sha);
    secp256k1_sha256_write(&sha, ell_a64, 64);
    secp256k1_sha256_write(&sha, ell_b64, 64);
    secp256k1_sha256_write(&sha, x32, 32);
    secp256k1_sha256_finalize(&sha, output);

    return 1;
}

const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_prefix = ellswift_xdh_hash_function_prefix;
const secp256k1_ellswift_xdh_hash_function secp256k1_ellswift_xdh_hash_function_bip324 = ellswift_xdh_hash_function_bip324;

int secp256k1_ellswift_xdh(const secp256k1_context *ctx, unsigned char *output, const unsigned char *ell_a64, const unsigned char *ell_b64, const unsigned char *seckey32, int party, secp256k1_ellswift_xdh_hash_function hashfp, void *data) {
    int ret = 0;
    int overflow;
    secp256k1_scalar s;
    secp256k1_fe xn, xd, px, u, t;
    unsigned char sx[32];
    const unsigned char* theirs64;

    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(output != NULL);
    ARG_CHECK(ell_a64 != NULL);
    ARG_CHECK(ell_b64 != NULL);
    ARG_CHECK(seckey32 != NULL);
    ARG_CHECK(hashfp != NULL);

    theirs64 = party ? ell_a64 : ell_b64;
    secp256k1_fe_set_b32_mod(&u, theirs64);
    secp256k1_fe_set_b32_mod(&t, theirs64 + 32);
    secp256k1_ellswift_xswiftec_frac_var(&xn, &xd, &u, &t);

    secp256k1_scalar_set_b32(&s, seckey32, &overflow);
    overflow = secp256k1_scalar_is_zero(&s);
    secp256k1_scalar_cmov(&s, &secp256k1_scalar_one, overflow);

    secp256k1_ecmult_const_xonly(&px, &xn, &xd, &s, 1);
    secp256k1_fe_normalize(&px);
    secp256k1_fe_get_b32(sx, &px);

    ret = hashfp(output, sx, ell_a64, ell_b64, data);

    memset(sx, 0, 32);
    secp256k1_fe_clear(&px);
    secp256k1_scalar_clear(&s);

    return !!ret & !overflow;
}

#endif
