#ifndef SECP256K1_ECMULT_GEN_COMPUTE_TABLE_IMPL_H
#define SECP256K1_ECMULT_GEN_COMPUTE_TABLE_IMPL_H

#include "ecmult_gen_compute_table.h"
#include "group_impl.h"
#include "field_impl.h"
#include "scalar_impl.h"
#include "ecmult_gen.h"
#include "util.h"

static void secp256k1_ecmult_gen_compute_table(secp256k1_ge_storage* table, const secp256k1_ge* gen, int blocks, int teeth, int spacing) {
    size_t points = ((size_t)1) << (teeth - 1);
    size_t points_total = points * blocks;
    secp256k1_ge* prec = checked_malloc(&default_error_callback, points_total * sizeof(*prec));
    secp256k1_gej* ds = checked_malloc(&default_error_callback, teeth * sizeof(*ds));
    secp256k1_gej* vs = checked_malloc(&default_error_callback, points_total * sizeof(*vs));
    secp256k1_gej u;
    size_t vs_pos = 0;
    secp256k1_scalar half;
    int block, i;

    VERIFY_CHECK(points_total > 0);

    secp256k1_scalar_half(&half, &secp256k1_scalar_one);
    secp256k1_gej_set_infinity(&u);
    for (i = 255; i >= 0; --i) {

        secp256k1_gej_double_var(&u, &u, NULL);
        if (secp256k1_scalar_get_bits_limb32(&half, i, 1)) {
            secp256k1_gej_add_ge_var(&u, &u, gen, NULL);
        }
    }
#ifdef VERIFY
    {

        secp256k1_gej double_u;
        secp256k1_gej_double_var(&double_u, &u, NULL);
        VERIFY_CHECK(secp256k1_gej_eq_ge_var(&double_u, gen));
    }
#endif

    for (block = 0; block < blocks; ++block) {
        int tooth;

        secp256k1_gej sum;
        secp256k1_gej_set_infinity(&sum);
        for (tooth = 0; tooth < teeth; ++tooth) {

            secp256k1_gej_add_var(&sum, &sum, &u, NULL);

            secp256k1_gej_double_var(&u, &u, NULL);

            ds[tooth] = u;

            if (block + tooth != blocks + teeth - 2) {
                int bit_off;
                for (bit_off = 1; bit_off < spacing; ++bit_off) {
                    secp256k1_gej_double_var(&u, &u, NULL);
                }
            }
        }

        secp256k1_gej_neg(&vs[vs_pos++], &sum);

        for (tooth = 0; tooth < teeth - 1; ++tooth) {
            size_t stride = ((size_t)1) << tooth;
            size_t index;
            for (index = 0; index < stride; ++index, ++vs_pos) {
                secp256k1_gej_add_var(&vs[vs_pos], &vs[vs_pos - stride], &ds[tooth], NULL);
            }
        }
    }
    VERIFY_CHECK(vs_pos == points_total);

    secp256k1_ge_set_all_gej_var(prec, vs, points_total);

    for (block = 0; block < blocks; ++block) {
        size_t index;
        for (index = 0; index < points; ++index) {
            VERIFY_CHECK(!secp256k1_ge_is_infinity(&prec[block * points + index]));
            secp256k1_ge_to_storage(&table[block * points + index], &prec[block * points + index]);
        }
    }

    free(vs);
    free(ds);
    free(prec);
}

#endif
