#ifndef SECP256K1_ECMULT_IMPL_H
#define SECP256K1_ECMULT_IMPL_H

#include <string.h>
#include <stdint.h>

#include "util.h"
#include "group.h"
#include "scalar.h"
#include "ecmult.h"
#include "precomputed_ecmult.h"

#if defined(EXHAUSTIVE_TEST_ORDER)

#  if EXHAUSTIVE_TEST_ORDER > 128
#    define WINDOW_A 5
#  elif EXHAUSTIVE_TEST_ORDER > 8
#    define WINDOW_A 4
#  else
#    define WINDOW_A 2
#  endif
#else

#  define WINDOW_A 5

#endif

#define WNAF_BITS 128
#define WNAF_SIZE_BITS(bits, w) CEIL_DIV(bits, w)
#define WNAF_SIZE(w) WNAF_SIZE_BITS(WNAF_BITS, w)

#define PIPPENGER_SCRATCH_OBJECTS 6
#define STRAUSS_SCRATCH_OBJECTS 5

#define PIPPENGER_MAX_BUCKET_WINDOW 12

#define ECMULT_PIPPENGER_THRESHOLD 88

#define ECMULT_MAX_POINTS_PER_BATCH 5000000

static void secp256k1_ecmult_odd_multiples_table(int n, secp256k1_ge *pre_a, secp256k1_fe *zr, secp256k1_fe *z, const secp256k1_gej *a) {
    secp256k1_gej d, ai;
    secp256k1_ge d_ge;
    int i;

    VERIFY_CHECK(!a->infinity);

    secp256k1_gej_double_var(&d, a, NULL);

    secp256k1_ge_set_xy(&d_ge, &d.x, &d.y);
    secp256k1_ge_set_gej_zinv(&pre_a[0], a, &d.z);
    secp256k1_gej_set_ge(&ai, &pre_a[0]);
    ai.z = a->z;

    zr[0] = d.z;

    for (i = 1; i < n; i++) {
        secp256k1_gej_add_ge_var(&ai, &ai, &d_ge, &zr[i]);
        secp256k1_ge_set_xy(&pre_a[i], &ai.x, &ai.y);
    }

    secp256k1_fe_mul(z, &ai.z, &d.z);
}

SECP256K1_INLINE static void secp256k1_ecmult_table_verify(int n, int w) {
    (void)n;
    (void)w;
    VERIFY_CHECK(((n) & 1) == 1);
    VERIFY_CHECK((n) >= -((1 << ((w)-1)) - 1));
    VERIFY_CHECK((n) <=  ((1 << ((w)-1)) - 1));
}

SECP256K1_INLINE static void secp256k1_ecmult_table_get_ge(secp256k1_ge *r, const secp256k1_ge *pre, int n, int w) {
    secp256k1_ecmult_table_verify(n,w);
    if (n > 0) {
        *r = pre[(n-1)/2];
    } else {
        *r = pre[(-n-1)/2];
        secp256k1_fe_negate(&(r->y), &(r->y), 1);
    }
}

SECP256K1_INLINE static void secp256k1_ecmult_table_get_ge_lambda(secp256k1_ge *r, const secp256k1_ge *pre, const secp256k1_fe *x, int n, int w) {
    secp256k1_ecmult_table_verify(n,w);
    if (n > 0) {
        secp256k1_ge_set_xy(r, &x[(n-1)/2], &pre[(n-1)/2].y);
    } else {
        secp256k1_ge_set_xy(r, &x[(-n-1)/2], &pre[(-n-1)/2].y);
        secp256k1_fe_negate(&(r->y), &(r->y), 1);
    }
}

SECP256K1_INLINE static void secp256k1_ecmult_table_get_ge_storage(secp256k1_ge *r, const secp256k1_ge_storage *pre, int n, int w) {
    secp256k1_ecmult_table_verify(n,w);
    if (n > 0) {
        secp256k1_ge_from_storage(r, &pre[(n-1)/2]);
    } else {
        secp256k1_ge_from_storage(r, &pre[(-n-1)/2]);
        secp256k1_fe_negate(&(r->y), &(r->y), 1);
    }
}

static int secp256k1_ecmult_wnaf(int *wnaf, int len, const secp256k1_scalar *a, int w) {
    secp256k1_scalar s;
    int last_set_bit = -1;
    int bit = 0;
    int sign = 1;
    int carry = 0;

    VERIFY_CHECK(wnaf != NULL);
    VERIFY_CHECK(0 <= len && len <= 256);
    VERIFY_CHECK(a != NULL);
    VERIFY_CHECK(2 <= w && w <= 31);

    memset(wnaf, 0, len * sizeof(wnaf[0]));

    s = *a;
    if (secp256k1_scalar_get_bits_limb32(&s, 255, 1)) {
        secp256k1_scalar_negate(&s, &s);
        sign = -1;
    }

    while (bit < len) {
        int now;
        int word;
        if (secp256k1_scalar_get_bits_limb32(&s, bit, 1) == (unsigned int)carry) {
            bit++;
            continue;
        }

        now = w;
        if (now > len - bit) {
            now = len - bit;
        }

        word = secp256k1_scalar_get_bits_var(&s, bit, now) + carry;

        carry = (word >> (w-1)) & 1;
        word -= carry << w;

        wnaf[bit] = sign * word;
        last_set_bit = bit;

        bit += now;
    }
#ifdef VERIFY
    {
        int verify_bit = bit;

        VERIFY_CHECK(carry == 0);

        while (verify_bit < 256) {
            VERIFY_CHECK(secp256k1_scalar_get_bits_limb32(&s, verify_bit, 1) == 0);
            verify_bit++;
        }
    }
#endif
    return last_set_bit + 1;
}

struct secp256k1_strauss_point_state {
    int wnaf_na_1[129];
    int wnaf_na_lam[129];
    int bits_na_1;
    int bits_na_lam;
};

struct secp256k1_strauss_state {

    secp256k1_fe* aux;
    secp256k1_ge* pre_a;
    struct secp256k1_strauss_point_state* ps;
};

static void secp256k1_ecmult_strauss_wnaf(const struct secp256k1_strauss_state *state, secp256k1_gej *r, size_t num, const secp256k1_gej *a, const secp256k1_scalar *na, const secp256k1_scalar *ng) {
    secp256k1_ge tmpa;
    secp256k1_fe Z;

    secp256k1_scalar ng_1, ng_128;
    int wnaf_ng_1[129];
    int bits_ng_1 = 0;
    int wnaf_ng_128[129];
    int bits_ng_128 = 0;
    int i;
    int bits = 0;
    size_t np;
    size_t no = 0;

    secp256k1_fe_set_int(&Z, 1);
    for (np = 0; np < num; ++np) {
        secp256k1_gej tmp;
        secp256k1_scalar na_1, na_lam;
        if (secp256k1_scalar_is_zero(&na[np]) || secp256k1_gej_is_infinity(&a[np])) {
            continue;
        }

        secp256k1_scalar_split_lambda(&na_1, &na_lam, &na[np]);

        state->ps[no].bits_na_1   = secp256k1_ecmult_wnaf(state->ps[no].wnaf_na_1,   129, &na_1,   WINDOW_A);
        state->ps[no].bits_na_lam = secp256k1_ecmult_wnaf(state->ps[no].wnaf_na_lam, 129, &na_lam, WINDOW_A);
        VERIFY_CHECK(state->ps[no].bits_na_1 <= 129);
        VERIFY_CHECK(state->ps[no].bits_na_lam <= 129);
        if (state->ps[no].bits_na_1 > bits) {
            bits = state->ps[no].bits_na_1;
        }
        if (state->ps[no].bits_na_lam > bits) {
            bits = state->ps[no].bits_na_lam;
        }

        tmp = a[np];
        if (no) {
            secp256k1_gej_rescale(&tmp, &Z);
        }
        secp256k1_ecmult_odd_multiples_table(ECMULT_TABLE_SIZE(WINDOW_A), state->pre_a + no * ECMULT_TABLE_SIZE(WINDOW_A), state->aux + no * ECMULT_TABLE_SIZE(WINDOW_A), &Z, &tmp);
        if (no) secp256k1_fe_mul(state->aux + no * ECMULT_TABLE_SIZE(WINDOW_A), state->aux + no * ECMULT_TABLE_SIZE(WINDOW_A), &(a[np].z));

        ++no;
    }

    if (no) {
        secp256k1_ge_table_set_globalz(ECMULT_TABLE_SIZE(WINDOW_A) * no, state->pre_a, state->aux);
    }

    for (np = 0; np < no; ++np) {
        for (i = 0; i < ECMULT_TABLE_SIZE(WINDOW_A); i++) {
            secp256k1_fe_mul(&state->aux[np * ECMULT_TABLE_SIZE(WINDOW_A) + i], &state->pre_a[np * ECMULT_TABLE_SIZE(WINDOW_A) + i].x, &secp256k1_const_beta);
        }
    }

    if (ng) {

        secp256k1_scalar_split_128(&ng_1, &ng_128, ng);

        bits_ng_1   = secp256k1_ecmult_wnaf(wnaf_ng_1,   129, &ng_1,   WINDOW_G);
        bits_ng_128 = secp256k1_ecmult_wnaf(wnaf_ng_128, 129, &ng_128, WINDOW_G);
        if (bits_ng_1 > bits) {
            bits = bits_ng_1;
        }
        if (bits_ng_128 > bits) {
            bits = bits_ng_128;
        }
    }

    secp256k1_gej_set_infinity(r);

    for (i = bits - 1; i >= 0; i--) {
        int n;
        secp256k1_gej_double_var(r, r, NULL);
        for (np = 0; np < no; ++np) {
            if (i < state->ps[np].bits_na_1 && (n = state->ps[np].wnaf_na_1[i])) {
                secp256k1_ecmult_table_get_ge(&tmpa, state->pre_a + np * ECMULT_TABLE_SIZE(WINDOW_A), n, WINDOW_A);
                secp256k1_gej_add_ge_var(r, r, &tmpa, NULL);
            }
            if (i < state->ps[np].bits_na_lam && (n = state->ps[np].wnaf_na_lam[i])) {
                secp256k1_ecmult_table_get_ge_lambda(&tmpa, state->pre_a + np * ECMULT_TABLE_SIZE(WINDOW_A), state->aux + np * ECMULT_TABLE_SIZE(WINDOW_A), n, WINDOW_A);
                secp256k1_gej_add_ge_var(r, r, &tmpa, NULL);
            }
        }
        if (i < bits_ng_1 && (n = wnaf_ng_1[i])) {
            secp256k1_ecmult_table_get_ge_storage(&tmpa, secp256k1_pre_g, n, WINDOW_G);
            secp256k1_gej_add_zinv_var(r, r, &tmpa, &Z);
        }
        if (i < bits_ng_128 && (n = wnaf_ng_128[i])) {
            secp256k1_ecmult_table_get_ge_storage(&tmpa, secp256k1_pre_g_128, n, WINDOW_G);
            secp256k1_gej_add_zinv_var(r, r, &tmpa, &Z);
        }
    }

    if (!r->infinity) {
        secp256k1_fe_mul(&r->z, &r->z, &Z);
    }
}

static void secp256k1_ecmult(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_scalar *na, const secp256k1_scalar *ng) {
    secp256k1_fe aux[ECMULT_TABLE_SIZE(WINDOW_A)];
    secp256k1_ge pre_a[ECMULT_TABLE_SIZE(WINDOW_A)];
    struct secp256k1_strauss_point_state ps[1];
    struct secp256k1_strauss_state state;

    state.aux = aux;
    state.pre_a = pre_a;
    state.ps = ps;
    secp256k1_ecmult_strauss_wnaf(&state, r, 1, a, na, ng);
}

static size_t secp256k1_strauss_scratch_size(size_t n_points) {
    static const size_t point_size = (sizeof(secp256k1_ge) + sizeof(secp256k1_fe)) * ECMULT_TABLE_SIZE(WINDOW_A) + sizeof(struct secp256k1_strauss_point_state) + sizeof(secp256k1_gej) + sizeof(secp256k1_scalar);
    return n_points*point_size;
}

static int secp256k1_ecmult_strauss_batch(const secp256k1_callback* error_callback, secp256k1_scratch *scratch, secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n_points, size_t cb_offset) {
    secp256k1_gej* points;
    secp256k1_scalar* scalars;
    struct secp256k1_strauss_state state;
    size_t i;
    const size_t scratch_checkpoint = secp256k1_scratch_checkpoint(error_callback, scratch);

    secp256k1_gej_set_infinity(r);
    if (inp_g_sc == NULL && n_points == 0) {
        return 1;
    }

    points = (secp256k1_gej*)secp256k1_scratch_alloc(error_callback, scratch, n_points * sizeof(secp256k1_gej));
    scalars = (secp256k1_scalar*)secp256k1_scratch_alloc(error_callback, scratch, n_points * sizeof(secp256k1_scalar));
    state.aux = (secp256k1_fe*)secp256k1_scratch_alloc(error_callback, scratch, n_points * ECMULT_TABLE_SIZE(WINDOW_A) * sizeof(secp256k1_fe));
    state.pre_a = (secp256k1_ge*)secp256k1_scratch_alloc(error_callback, scratch, n_points * ECMULT_TABLE_SIZE(WINDOW_A) * sizeof(secp256k1_ge));
    state.ps = (struct secp256k1_strauss_point_state*)secp256k1_scratch_alloc(error_callback, scratch, n_points * sizeof(struct secp256k1_strauss_point_state));

    if (points == NULL || scalars == NULL || state.aux == NULL || state.pre_a == NULL || state.ps == NULL) {
        secp256k1_scratch_apply_checkpoint(error_callback, scratch, scratch_checkpoint);
        return 0;
    }

    for (i = 0; i < n_points; i++) {
        secp256k1_ge point;
        if (!cb(&scalars[i], &point, i+cb_offset, cbdata)) {
            secp256k1_scratch_apply_checkpoint(error_callback, scratch, scratch_checkpoint);
            return 0;
        }
        secp256k1_gej_set_ge(&points[i], &point);
    }
    secp256k1_ecmult_strauss_wnaf(&state, r, n_points, points, scalars, inp_g_sc);
    secp256k1_scratch_apply_checkpoint(error_callback, scratch, scratch_checkpoint);
    return 1;
}

static int secp256k1_ecmult_strauss_batch_single(const secp256k1_callback* error_callback, secp256k1_scratch *scratch, secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n) {
    return secp256k1_ecmult_strauss_batch(error_callback, scratch, r, inp_g_sc, cb, cbdata, n, 0);
}

static size_t secp256k1_strauss_max_points(const secp256k1_callback* error_callback, secp256k1_scratch *scratch) {
    return secp256k1_scratch_max_allocation(error_callback, scratch, STRAUSS_SCRATCH_OBJECTS) / secp256k1_strauss_scratch_size(1);
}

static int secp256k1_wnaf_fixed(int *wnaf, const secp256k1_scalar *s, int w) {
    int skew = 0;
    int pos;
    int max_pos;
    int last_w;
    const secp256k1_scalar *work = s;

    if (secp256k1_scalar_is_zero(s)) {
        for (pos = 0; pos < WNAF_SIZE(w); pos++) {
            wnaf[pos] = 0;
        }
        return 0;
    }

    if (secp256k1_scalar_is_even(s)) {
        skew = 1;
    }

    wnaf[0] = secp256k1_scalar_get_bits_var(work, 0, w) + skew;

    last_w = WNAF_BITS - (WNAF_SIZE(w) - 1) * w;

    for (pos = WNAF_SIZE(w) - 1; pos > 0; pos--) {
        int val = secp256k1_scalar_get_bits_var(work, pos * w, pos == WNAF_SIZE(w)-1 ? last_w : w);
        if(val != 0) {
            break;
        }
        wnaf[pos] = 0;
    }
    max_pos = pos;
    pos = 1;

    while (pos <= max_pos) {
        int val = secp256k1_scalar_get_bits_var(work, pos * w, pos == WNAF_SIZE(w)-1 ? last_w : w);
        if ((val & 1) == 0) {
            wnaf[pos - 1] -= (1 << w);
            wnaf[pos] = (val + 1);
        } else {
            wnaf[pos] = val;
        }

        if (pos >= 2 && ((wnaf[pos - 1] == 1 && wnaf[pos - 2] < 0) || (wnaf[pos - 1] == -1 && wnaf[pos - 2] > 0))) {
            if (wnaf[pos - 1] == 1) {
                wnaf[pos - 2] += 1 << w;
            } else {
                wnaf[pos - 2] -= 1 << w;
            }
            wnaf[pos - 1] = 0;
        }
        ++pos;
    }

    return skew;
}

struct secp256k1_pippenger_point_state {
    int skew_na;
    size_t input_pos;
};

struct secp256k1_pippenger_state {
    int *wnaf_na;
    struct secp256k1_pippenger_point_state* ps;
};

static int secp256k1_ecmult_pippenger_wnaf(secp256k1_gej *buckets, int bucket_window, struct secp256k1_pippenger_state *state, secp256k1_gej *r, const secp256k1_scalar *sc, const secp256k1_ge *pt, size_t num) {
    size_t n_wnaf = WNAF_SIZE(bucket_window+1);
    size_t np;
    size_t no = 0;
    int i;
    int j;

    for (np = 0; np < num; ++np) {
        if (secp256k1_scalar_is_zero(&sc[np]) || secp256k1_ge_is_infinity(&pt[np])) {
            continue;
        }
        state->ps[no].input_pos = np;
        state->ps[no].skew_na = secp256k1_wnaf_fixed(&state->wnaf_na[no*n_wnaf], &sc[np], bucket_window+1);
        no++;
    }
    secp256k1_gej_set_infinity(r);

    if (no == 0) {
        return 1;
    }

    for (i = n_wnaf - 1; i >= 0; i--) {
        secp256k1_gej running_sum;

        for(j = 0; j < ECMULT_TABLE_SIZE(bucket_window+2); j++) {
            secp256k1_gej_set_infinity(&buckets[j]);
        }

        for (np = 0; np < no; ++np) {
            int n = state->wnaf_na[np*n_wnaf + i];
            struct secp256k1_pippenger_point_state point_state = state->ps[np];
            secp256k1_ge tmp;
            int idx;

            if (i == 0) {

                int skew = point_state.skew_na;
                if (skew) {
                    secp256k1_ge_neg(&tmp, &pt[point_state.input_pos]);
                    secp256k1_gej_add_ge_var(&buckets[0], &buckets[0], &tmp, NULL);
                }
            }
            if (n > 0) {
                idx = (n - 1)/2;
                secp256k1_gej_add_ge_var(&buckets[idx], &buckets[idx], &pt[point_state.input_pos], NULL);
            } else if (n < 0) {
                idx = -(n + 1)/2;
                secp256k1_ge_neg(&tmp, &pt[point_state.input_pos]);
                secp256k1_gej_add_ge_var(&buckets[idx], &buckets[idx], &tmp, NULL);
            }
        }

        for(j = 0; j < bucket_window; j++) {
            secp256k1_gej_double_var(r, r, NULL);
        }

        secp256k1_gej_set_infinity(&running_sum);

        for(j = ECMULT_TABLE_SIZE(bucket_window+2) - 1; j > 0; j--) {
            secp256k1_gej_add_var(&running_sum, &running_sum, &buckets[j], NULL);
            secp256k1_gej_add_var(r, r, &running_sum, NULL);
        }

        secp256k1_gej_add_var(&running_sum, &running_sum, &buckets[0], NULL);
        secp256k1_gej_double_var(r, r, NULL);
        secp256k1_gej_add_var(r, r, &running_sum, NULL);
    }
    return 1;
}

static int secp256k1_pippenger_bucket_window(size_t n) {
    if (n <= 1) {
        return 1;
    } else if (n <= 4) {
        return 2;
    } else if (n <= 20) {
        return 3;
    } else if (n <= 57) {
        return 4;
    } else if (n <= 136) {
        return 5;
    } else if (n <= 235) {
        return 6;
    } else if (n <= 1260) {
        return 7;
    } else if (n <= 4420) {
        return 9;
    } else if (n <= 7880) {
        return 10;
    } else if (n <= 16050) {
        return 11;
    } else {
        return PIPPENGER_MAX_BUCKET_WINDOW;
    }
}

static size_t secp256k1_pippenger_bucket_window_inv(int bucket_window) {
    switch(bucket_window) {
        case 1: return 1;
        case 2: return 4;
        case 3: return 20;
        case 4: return 57;
        case 5: return 136;
        case 6: return 235;
        case 7: return 1260;
        case 8: return 1260;
        case 9: return 4420;
        case 10: return 7880;
        case 11: return 16050;
        case PIPPENGER_MAX_BUCKET_WINDOW: return SIZE_MAX;
    }
    return 0;
}

SECP256K1_INLINE static void secp256k1_ecmult_endo_split(secp256k1_scalar *s1, secp256k1_scalar *s2, secp256k1_ge *p1, secp256k1_ge *p2) {
    secp256k1_scalar tmp = *s1;
    secp256k1_scalar_split_lambda(s1, s2, &tmp);
    secp256k1_ge_mul_lambda(p2, p1);

    if (secp256k1_scalar_is_high(s1)) {
        secp256k1_scalar_negate(s1, s1);
        secp256k1_ge_neg(p1, p1);
    }
    if (secp256k1_scalar_is_high(s2)) {
        secp256k1_scalar_negate(s2, s2);
        secp256k1_ge_neg(p2, p2);
    }
}

static size_t secp256k1_pippenger_scratch_size(size_t n_points, int bucket_window) {
    size_t entries = 2*n_points + 2;
    size_t entry_size = sizeof(secp256k1_ge) + sizeof(secp256k1_scalar) + sizeof(struct secp256k1_pippenger_point_state) + (WNAF_SIZE(bucket_window+1)+1)*sizeof(int);
    return (sizeof(secp256k1_gej) << bucket_window) + sizeof(struct secp256k1_pippenger_state) + entries * entry_size;
}

static int secp256k1_ecmult_pippenger_batch(const secp256k1_callback* error_callback, secp256k1_scratch *scratch, secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n_points, size_t cb_offset) {
    const size_t scratch_checkpoint = secp256k1_scratch_checkpoint(error_callback, scratch);

    size_t entries = 2*n_points + 2;
    secp256k1_ge *points;
    secp256k1_scalar *scalars;
    secp256k1_gej *buckets;
    struct secp256k1_pippenger_state *state_space;
    size_t idx = 0;
    size_t point_idx = 0;
    int i, j;
    int bucket_window;

    secp256k1_gej_set_infinity(r);
    if (inp_g_sc == NULL && n_points == 0) {
        return 1;
    }
    bucket_window = secp256k1_pippenger_bucket_window(n_points);

    points = (secp256k1_ge *) secp256k1_scratch_alloc(error_callback, scratch, entries * sizeof(*points));
    scalars = (secp256k1_scalar *) secp256k1_scratch_alloc(error_callback, scratch, entries * sizeof(*scalars));
    state_space = (struct secp256k1_pippenger_state *) secp256k1_scratch_alloc(error_callback, scratch, sizeof(*state_space));
    if (points == NULL || scalars == NULL || state_space == NULL) {
        secp256k1_scratch_apply_checkpoint(error_callback, scratch, scratch_checkpoint);
        return 0;
    }
    state_space->ps = (struct secp256k1_pippenger_point_state *) secp256k1_scratch_alloc(error_callback, scratch, entries * sizeof(*state_space->ps));
    state_space->wnaf_na = (int *) secp256k1_scratch_alloc(error_callback, scratch, entries*(WNAF_SIZE(bucket_window+1)) * sizeof(int));
    buckets = (secp256k1_gej *) secp256k1_scratch_alloc(error_callback, scratch, ((size_t)1 << bucket_window) * sizeof(*buckets));
    if (state_space->ps == NULL || state_space->wnaf_na == NULL || buckets == NULL) {
        secp256k1_scratch_apply_checkpoint(error_callback, scratch, scratch_checkpoint);
        return 0;
    }

    if (inp_g_sc != NULL) {
        scalars[0] = *inp_g_sc;
        points[0] = secp256k1_ge_const_g;
        idx++;
        secp256k1_ecmult_endo_split(&scalars[0], &scalars[1], &points[0], &points[1]);
        idx++;
    }

    while (point_idx < n_points) {
        if (!cb(&scalars[idx], &points[idx], point_idx + cb_offset, cbdata)) {
            secp256k1_scratch_apply_checkpoint(error_callback, scratch, scratch_checkpoint);
            return 0;
        }
        idx++;
        secp256k1_ecmult_endo_split(&scalars[idx - 1], &scalars[idx], &points[idx - 1], &points[idx]);
        idx++;
        point_idx++;
    }

    secp256k1_ecmult_pippenger_wnaf(buckets, bucket_window, state_space, r, scalars, points, idx);

    for(i = 0; (size_t)i < idx; i++) {
        secp256k1_scalar_clear(&scalars[i]);
        state_space->ps[i].skew_na = 0;
        for(j = 0; j < WNAF_SIZE(bucket_window+1); j++) {
            state_space->wnaf_na[i * WNAF_SIZE(bucket_window+1) + j] = 0;
        }
    }
    for(i = 0; i < 1<<bucket_window; i++) {
        secp256k1_gej_clear(&buckets[i]);
    }
    secp256k1_scratch_apply_checkpoint(error_callback, scratch, scratch_checkpoint);
    return 1;
}

static int secp256k1_ecmult_pippenger_batch_single(const secp256k1_callback* error_callback, secp256k1_scratch *scratch, secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n) {
    return secp256k1_ecmult_pippenger_batch(error_callback, scratch, r, inp_g_sc, cb, cbdata, n, 0);
}

static size_t secp256k1_pippenger_max_points(const secp256k1_callback* error_callback, secp256k1_scratch *scratch) {
    size_t max_alloc = secp256k1_scratch_max_allocation(error_callback, scratch, PIPPENGER_SCRATCH_OBJECTS);
    int bucket_window;
    size_t res = 0;

    for (bucket_window = 1; bucket_window <= PIPPENGER_MAX_BUCKET_WINDOW; bucket_window++) {
        size_t n_points;
        size_t max_points = secp256k1_pippenger_bucket_window_inv(bucket_window);
        size_t space_for_points;
        size_t space_overhead;
        size_t entry_size = sizeof(secp256k1_ge) + sizeof(secp256k1_scalar) + sizeof(struct secp256k1_pippenger_point_state) + (WNAF_SIZE(bucket_window+1)+1)*sizeof(int);

        entry_size = 2*entry_size;
        space_overhead = (sizeof(secp256k1_gej) << bucket_window) + entry_size + sizeof(struct secp256k1_pippenger_state);
        if (space_overhead > max_alloc) {
            break;
        }
        space_for_points = max_alloc - space_overhead;

        n_points = space_for_points/entry_size;
        n_points = n_points > max_points ? max_points : n_points;
        if (n_points > res) {
            res = n_points;
        }
        if (n_points < max_points) {

            break;
        }
    }
    return res;
}

static int secp256k1_ecmult_multi_simple_var(secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n_points) {
    size_t point_idx;
    secp256k1_gej tmpj;

    secp256k1_gej_set_infinity(r);
    secp256k1_gej_set_infinity(&tmpj);

    secp256k1_ecmult(r, &tmpj, &secp256k1_scalar_zero, inp_g_sc);
    for (point_idx = 0; point_idx < n_points; point_idx++) {
        secp256k1_ge point;
        secp256k1_gej pointj;
        secp256k1_scalar scalar;
        if (!cb(&scalar, &point, point_idx, cbdata)) {
            return 0;
        }

        secp256k1_gej_set_ge(&pointj, &point);
        secp256k1_ecmult(&tmpj, &pointj, &scalar, NULL);
        secp256k1_gej_add_var(r, r, &tmpj, NULL);
    }
    return 1;
}

static int secp256k1_ecmult_multi_batch_size_helper(size_t *n_batches, size_t *n_batch_points, size_t max_n_batch_points, size_t n) {
    if (max_n_batch_points == 0) {
        return 0;
    }
    if (max_n_batch_points > ECMULT_MAX_POINTS_PER_BATCH) {
        max_n_batch_points = ECMULT_MAX_POINTS_PER_BATCH;
    }
    if (n == 0) {
        *n_batches = 0;
        *n_batch_points = 0;
        return 1;
    }

    *n_batches = CEIL_DIV(n, max_n_batch_points);
    *n_batch_points = CEIL_DIV(n, *n_batches);
    return 1;
}

typedef int (*secp256k1_ecmult_multi_func)(const secp256k1_callback* error_callback, secp256k1_scratch*, secp256k1_gej*, const secp256k1_scalar*, secp256k1_ecmult_multi_callback cb, void*, size_t);
static int secp256k1_ecmult_multi_var(const secp256k1_callback* error_callback, secp256k1_scratch *scratch, secp256k1_gej *r, const secp256k1_scalar *inp_g_sc, secp256k1_ecmult_multi_callback cb, void *cbdata, size_t n) {
    size_t i;

    int (*f)(const secp256k1_callback* error_callback, secp256k1_scratch*, secp256k1_gej*, const secp256k1_scalar*, secp256k1_ecmult_multi_callback cb, void*, size_t, size_t);
    size_t n_batches;
    size_t n_batch_points;

    secp256k1_gej_set_infinity(r);
    if (inp_g_sc == NULL && n == 0) {
        return 1;
    } else if (n == 0) {
        secp256k1_ecmult(r, r, &secp256k1_scalar_zero, inp_g_sc);
        return 1;
    }
    if (scratch == NULL) {
        return secp256k1_ecmult_multi_simple_var(r, inp_g_sc, cb, cbdata, n);
    }

    if (!secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, secp256k1_pippenger_max_points(error_callback, scratch), n)) {
        return secp256k1_ecmult_multi_simple_var(r, inp_g_sc, cb, cbdata, n);
    }
    if (n_batch_points >= ECMULT_PIPPENGER_THRESHOLD) {
        f = secp256k1_ecmult_pippenger_batch;
    } else {
        if (!secp256k1_ecmult_multi_batch_size_helper(&n_batches, &n_batch_points, secp256k1_strauss_max_points(error_callback, scratch), n)) {
            return secp256k1_ecmult_multi_simple_var(r, inp_g_sc, cb, cbdata, n);
        }
        f = secp256k1_ecmult_strauss_batch;
    }
    for(i = 0; i < n_batches; i++) {
        size_t nbp = n < n_batch_points ? n : n_batch_points;
        size_t offset = n_batch_points*i;
        secp256k1_gej tmp;
        if (!f(error_callback, scratch, &tmp, i == 0 ? inp_g_sc : NULL, cb, cbdata, nbp, offset)) {
            return 0;
        }
        secp256k1_gej_add_var(r, r, &tmp, NULL);
        n -= nbp;
    }
    return 1;
}

#endif
