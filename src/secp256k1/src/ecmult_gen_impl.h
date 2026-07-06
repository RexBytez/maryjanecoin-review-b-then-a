#ifndef SECP256K1_ECMULT_GEN_IMPL_H
#define SECP256K1_ECMULT_GEN_IMPL_H

#include "util.h"
#include "scalar.h"
#include "group.h"
#include "ecmult_gen.h"
#include "hash_impl.h"
#include "precomputed_ecmult_gen.h"

static void secp256k1_ecmult_gen_context_build(secp256k1_ecmult_gen_context *ctx) {
    secp256k1_ecmult_gen_blind(ctx, NULL);
    ctx->built = 1;
}

static int secp256k1_ecmult_gen_context_is_built(const secp256k1_ecmult_gen_context* ctx) {
    return ctx->built;
}

static void secp256k1_ecmult_gen_context_clear(secp256k1_ecmult_gen_context *ctx) {
    ctx->built = 0;
    secp256k1_scalar_clear(&ctx->scalar_offset);
    secp256k1_ge_clear(&ctx->ge_offset);
    secp256k1_fe_clear(&ctx->proj_blind);
}

static void secp256k1_ecmult_gen_scalar_diff(secp256k1_scalar* diff) {
    int i;

    secp256k1_scalar neghalf;
    secp256k1_scalar_half(&neghalf, &secp256k1_scalar_one);
    secp256k1_scalar_negate(&neghalf, &neghalf);

    *diff = secp256k1_scalar_one;
    for (i = 0; i < COMB_BITS - 1; ++i) {
        secp256k1_scalar_add(diff, diff, diff);
    }

    secp256k1_scalar_add(diff, diff, &neghalf);
}

static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context *ctx, secp256k1_gej *r, const secp256k1_scalar *gn) {
    uint32_t comb_off;
    secp256k1_ge add;
    secp256k1_fe neg;
    secp256k1_ge_storage adds;
    secp256k1_scalar d;

    uint32_t recoded[(COMB_BITS + 31) >> 5] = {0};
    int first = 1, i;

    memset(&adds, 0, sizeof(adds));

    secp256k1_scalar_add(&d, &ctx->scalar_offset, gn);

    for (i = 0; i < 8 && i < ((COMB_BITS + 31) >> 5); ++i) {
        recoded[i] = secp256k1_scalar_get_bits_limb32(&d, 32 * i, 32);
    }
    secp256k1_scalar_clear(&d);

    comb_off = COMB_SPACING - 1;
    while (1) {
        uint32_t block;
        uint32_t bit_pos = comb_off;

        for (block = 0; block < COMB_BLOCKS; ++block) {

            uint32_t bits = 0, sign, abs, index, tooth;

            for (tooth = 0; tooth < COMB_TEETH; ++tooth) {

                uint32_t bitdata = secp256k1_rotr32(recoded[bit_pos >> 5], bit_pos & 0x1f);

                uint32_t volatile vmask = ~(1 << tooth);
                bits &= vmask;

                bits ^= bitdata << tooth;
                bit_pos += COMB_SPACING;
            }

            sign = (bits >> (COMB_TEETH - 1)) & 1;
            abs = (bits ^ -sign) & (COMB_POINTS - 1);
            VERIFY_CHECK(sign == 0 || sign == 1);
            VERIFY_CHECK(abs < COMB_POINTS);

            for (index = 0; index < COMB_POINTS; ++index) {
                secp256k1_ge_storage_cmov(&adds, &secp256k1_ecmult_gen_prec_table[block][index], index == abs);
            }

            secp256k1_ge_from_storage(&add, &adds);
            secp256k1_fe_negate(&neg, &add.y, 1);
            secp256k1_fe_cmov(&add.y, &neg, sign);

            if (EXPECT(first, 0)) {

                secp256k1_gej_set_ge(r, &add);

                secp256k1_gej_rescale(r, &ctx->proj_blind);
                first = 0;
            } else {
                secp256k1_gej_add_ge(r, r, &add);
            }
        }

        if (comb_off-- == 0) break;
        secp256k1_gej_double(r, r);
    }

    secp256k1_gej_add_ge(r, r, &ctx->ge_offset);

    secp256k1_fe_clear(&neg);
    secp256k1_ge_clear(&add);
    memset(&adds, 0, sizeof(adds));
    memset(&recoded, 0, sizeof(recoded));
}

static void secp256k1_ecmult_gen_blind(secp256k1_ecmult_gen_context *ctx, const unsigned char *seed32) {
    secp256k1_scalar b;
    secp256k1_scalar diff;
    secp256k1_gej gb;
    secp256k1_fe f;
    unsigned char nonce32[32];
    secp256k1_rfc6979_hmac_sha256 rng;
    unsigned char keydata[64];

    secp256k1_ecmult_gen_scalar_diff(&diff);

    if (seed32 == NULL) {

        secp256k1_ge_neg(&ctx->ge_offset, &secp256k1_ge_const_g);
        secp256k1_scalar_add(&ctx->scalar_offset, &secp256k1_scalar_one, &diff);
        ctx->proj_blind = secp256k1_fe_one;
        return;
    }

    secp256k1_scalar_get_b32(keydata, &ctx->scalar_offset);

    VERIFY_CHECK(seed32 != NULL);
    memcpy(keydata + 32, seed32, 32);
    secp256k1_rfc6979_hmac_sha256_initialize(&rng, keydata, 64);
    memset(keydata, 0, sizeof(keydata));

    secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
    secp256k1_fe_set_b32_mod(&f, nonce32);
    secp256k1_fe_cmov(&f, &secp256k1_fe_one, secp256k1_fe_normalizes_to_zero(&f));
    ctx->proj_blind = f;

    secp256k1_rfc6979_hmac_sha256_generate(&rng, nonce32, 32);
    secp256k1_scalar_set_b32(&b, nonce32, NULL);

    secp256k1_scalar_cmov(&b, &secp256k1_scalar_one, secp256k1_scalar_is_zero(&b));
    secp256k1_rfc6979_hmac_sha256_finalize(&rng);
    memset(nonce32, 0, 32);
    secp256k1_ecmult_gen(ctx, &gb, &b);
    secp256k1_scalar_negate(&b, &b);
    secp256k1_scalar_add(&ctx->scalar_offset, &b, &diff);
    secp256k1_ge_set_gej(&ctx->ge_offset, &gb);

    secp256k1_scalar_clear(&b);
    secp256k1_gej_clear(&gb);
    secp256k1_fe_clear(&f);
}

#endif
