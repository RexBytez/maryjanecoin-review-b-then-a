#ifndef SECP256K1_MODULE_RECOVERY_EXHAUSTIVE_TESTS_H
#define SECP256K1_MODULE_RECOVERY_EXHAUSTIVE_TESTS_H

#include "main_impl.h"
#include "../../../include/secp256k1_recovery.h"

static void test_exhaustive_recovery_sign(const secp256k1_context *ctx, const secp256k1_ge *group) {
    int i, j, k;
    uint64_t iter = 0;

    for (i = 1; i < EXHAUSTIVE_TEST_ORDER; i++) {
        for (j = 1; j < EXHAUSTIVE_TEST_ORDER; j++) {
            if (skip_section(&iter)) continue;
            for (k = 1; k < EXHAUSTIVE_TEST_ORDER; k++) {
                const int starting_k = k;
                secp256k1_fe r_dot_y_normalized;
                secp256k1_ecdsa_recoverable_signature rsig;
                secp256k1_ecdsa_signature sig;
                secp256k1_scalar sk, msg, r, s, expected_r;
                unsigned char sk32[32], msg32[32];
                int expected_recid;
                int recid;
                int overflow;
                secp256k1_scalar_set_int(&msg, i);
                secp256k1_scalar_set_int(&sk, j);
                secp256k1_scalar_get_b32(sk32, &sk);
                secp256k1_scalar_get_b32(msg32, &msg);

                secp256k1_ecdsa_sign_recoverable(ctx, &rsig, msg32, sk32, secp256k1_nonce_function_smallint, &k);

                secp256k1_ecdsa_recoverable_signature_load(ctx, &r, &s, &recid, &rsig);
                r_from_k(&expected_r, group, k, &overflow);
                CHECK(r == expected_r);
                CHECK((k * s) % EXHAUSTIVE_TEST_ORDER == (i + r * j) % EXHAUSTIVE_TEST_ORDER ||
                      (k * (EXHAUSTIVE_TEST_ORDER - s)) % EXHAUSTIVE_TEST_ORDER == (i + r * j) % EXHAUSTIVE_TEST_ORDER);

                expected_recid = overflow ? 2 : 0;
                r_dot_y_normalized = group[k].y;
                secp256k1_fe_normalize(&r_dot_y_normalized);

                if ((k * s) % EXHAUSTIVE_TEST_ORDER == (i + r * j) % EXHAUSTIVE_TEST_ORDER) {
                    expected_recid |= secp256k1_fe_is_odd(&r_dot_y_normalized);
                } else {
                    expected_recid |= !secp256k1_fe_is_odd(&r_dot_y_normalized);
                }
                CHECK(recid == expected_recid);

                secp256k1_ecdsa_recoverable_signature_convert(ctx, &sig, &rsig);
                secp256k1_ecdsa_signature_load(ctx, &r, &s, &sig);

                r_from_k(&expected_r, group, k, NULL);
                CHECK(r == expected_r);
                CHECK((k * s) % EXHAUSTIVE_TEST_ORDER == (i + r * j) % EXHAUSTIVE_TEST_ORDER ||
                      (k * (EXHAUSTIVE_TEST_ORDER - s)) % EXHAUSTIVE_TEST_ORDER == (i + r * j) % EXHAUSTIVE_TEST_ORDER);

                if (k < starting_k) {
                    break;
                }
            }
        }
    }
}

static void test_exhaustive_recovery_verify(const secp256k1_context *ctx, const secp256k1_ge *group) {

    int s, r, msg, key;
    uint64_t iter = 0;
    for (s = 1; s < EXHAUSTIVE_TEST_ORDER; s++) {
        for (r = 1; r < EXHAUSTIVE_TEST_ORDER; r++) {
            for (msg = 1; msg < EXHAUSTIVE_TEST_ORDER; msg++) {
                for (key = 1; key < EXHAUSTIVE_TEST_ORDER; key++) {
                    secp256k1_ge nonconst_ge;
                    secp256k1_ecdsa_recoverable_signature rsig;
                    secp256k1_ecdsa_signature sig;
                    secp256k1_pubkey pk;
                    secp256k1_scalar sk_s, msg_s, r_s, s_s;
                    secp256k1_scalar s_times_k_s, msg_plus_r_times_sk_s;
                    int recid = 0;
                    int k, should_verify;
                    unsigned char msg32[32];

                    if (skip_section(&iter)) continue;

                    secp256k1_scalar_set_int(&s_s, s);
                    secp256k1_scalar_set_int(&r_s, r);
                    secp256k1_scalar_set_int(&msg_s, msg);
                    secp256k1_scalar_set_int(&sk_s, key);
                    secp256k1_scalar_get_b32(msg32, &msg_s);

                    should_verify = 0;
                    for (k = 0; k < EXHAUSTIVE_TEST_ORDER; k++) {
                        secp256k1_scalar check_x_s;
                        r_from_k(&check_x_s, group, k, NULL);
                        if (r_s == check_x_s) {
                            secp256k1_scalar_set_int(&s_times_k_s, k);
                            secp256k1_scalar_mul(&s_times_k_s, &s_times_k_s, &s_s);
                            secp256k1_scalar_mul(&msg_plus_r_times_sk_s, &r_s, &sk_s);
                            secp256k1_scalar_add(&msg_plus_r_times_sk_s, &msg_plus_r_times_sk_s, &msg_s);
                            should_verify |= secp256k1_scalar_eq(&s_times_k_s, &msg_plus_r_times_sk_s);
                        }
                    }

                    should_verify &= !secp256k1_scalar_is_high(&s_s);

                    secp256k1_ecdsa_recoverable_signature_save(&rsig, &r_s, &s_s, recid);
                    secp256k1_ecdsa_recoverable_signature_convert(ctx, &sig, &rsig);
                    memcpy(&nonconst_ge, &group[sk_s], sizeof(nonconst_ge));
                    secp256k1_pubkey_save(&pk, &nonconst_ge);
                    CHECK(should_verify ==
                          secp256k1_ecdsa_verify(ctx, &sig, msg32, &pk));
                }
            }
        }
    }
}

static void test_exhaustive_recovery(const secp256k1_context *ctx, const secp256k1_ge *group) {
    test_exhaustive_recovery_sign(ctx, group);
    test_exhaustive_recovery_verify(ctx, group);
}

#endif
