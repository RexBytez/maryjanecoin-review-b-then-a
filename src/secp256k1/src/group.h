#ifndef SECP256K1_GROUP_H
#define SECP256K1_GROUP_H

#include "field.h"

typedef struct {
    secp256k1_fe x;
    secp256k1_fe y;
    int infinity;
} secp256k1_ge;

#define SECP256K1_GE_CONST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {SECP256K1_FE_CONST((a),(b),(c),(d),(e),(f),(g),(h)), SECP256K1_FE_CONST((i),(j),(k),(l),(m),(n),(o),(p)), 0}
#define SECP256K1_GE_CONST_INFINITY {SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), 1}

typedef struct {
    secp256k1_fe x;
    secp256k1_fe y;
    secp256k1_fe z;
    int infinity;
} secp256k1_gej;

#define SECP256K1_GEJ_CONST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {SECP256K1_FE_CONST((a),(b),(c),(d),(e),(f),(g),(h)), SECP256K1_FE_CONST((i),(j),(k),(l),(m),(n),(o),(p)), SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 1), 0}
#define SECP256K1_GEJ_CONST_INFINITY {SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), SECP256K1_FE_CONST(0, 0, 0, 0, 0, 0, 0, 0), 1}

typedef struct {
    secp256k1_fe_storage x;
    secp256k1_fe_storage y;
} secp256k1_ge_storage;

#define SECP256K1_GE_STORAGE_CONST(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {SECP256K1_FE_STORAGE_CONST((a),(b),(c),(d),(e),(f),(g),(h)), SECP256K1_FE_STORAGE_CONST((i),(j),(k),(l),(m),(n),(o),(p))}

#define SECP256K1_GE_STORAGE_CONST_GET(t) SECP256K1_FE_STORAGE_CONST_GET(t.x), SECP256K1_FE_STORAGE_CONST_GET(t.y)

#define SECP256K1_GE_X_MAGNITUDE_MAX  4
#define SECP256K1_GE_Y_MAGNITUDE_MAX  3
#define SECP256K1_GEJ_X_MAGNITUDE_MAX 4
#define SECP256K1_GEJ_Y_MAGNITUDE_MAX 4
#define SECP256K1_GEJ_Z_MAGNITUDE_MAX 1

static void secp256k1_ge_set_xy(secp256k1_ge *r, const secp256k1_fe *x, const secp256k1_fe *y);

static int secp256k1_ge_set_xo_var(secp256k1_ge *r, const secp256k1_fe *x, int odd);

static int secp256k1_ge_x_on_curve_var(const secp256k1_fe *x);

static int secp256k1_ge_x_frac_on_curve_var(const secp256k1_fe *xn, const secp256k1_fe *xd);

static int secp256k1_ge_is_infinity(const secp256k1_ge *a);

static int secp256k1_ge_is_valid_var(const secp256k1_ge *a);

static void secp256k1_ge_neg(secp256k1_ge *r, const secp256k1_ge *a);

static void secp256k1_ge_set_gej(secp256k1_ge *r, secp256k1_gej *a);

static void secp256k1_ge_set_gej_var(secp256k1_ge *r, secp256k1_gej *a);

static void secp256k1_ge_set_all_gej_var(secp256k1_ge *r, const secp256k1_gej *a, size_t len);

static void secp256k1_ge_table_set_globalz(size_t len, secp256k1_ge *a, const secp256k1_fe *zr);

static int secp256k1_ge_eq_var(const secp256k1_ge *a, const secp256k1_ge *b);

static void secp256k1_ge_set_infinity(secp256k1_ge *r);

static void secp256k1_gej_set_infinity(secp256k1_gej *r);

static void secp256k1_gej_set_ge(secp256k1_gej *r, const secp256k1_ge *a);

static int secp256k1_gej_eq_var(const secp256k1_gej *a, const secp256k1_gej *b);

static int secp256k1_gej_eq_ge_var(const secp256k1_gej *a, const secp256k1_ge *b);

static int secp256k1_gej_eq_x_var(const secp256k1_fe *x, const secp256k1_gej *a);

static void secp256k1_gej_neg(secp256k1_gej *r, const secp256k1_gej *a);

static int secp256k1_gej_is_infinity(const secp256k1_gej *a);

static void secp256k1_gej_double(secp256k1_gej *r, const secp256k1_gej *a);

static void secp256k1_gej_double_var(secp256k1_gej *r, const secp256k1_gej *a, secp256k1_fe *rzr);

static void secp256k1_gej_add_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_gej *b, secp256k1_fe *rzr);

static void secp256k1_gej_add_ge(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b);

static void secp256k1_gej_add_ge_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b, secp256k1_fe *rzr);

static void secp256k1_gej_add_zinv_var(secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_ge *b, const secp256k1_fe *bzinv);

static void secp256k1_ge_mul_lambda(secp256k1_ge *r, const secp256k1_ge *a);

static void secp256k1_gej_clear(secp256k1_gej *r);

static void secp256k1_ge_clear(secp256k1_ge *r);

static void secp256k1_ge_to_storage(secp256k1_ge_storage *r, const secp256k1_ge *a);

static void secp256k1_ge_from_storage(secp256k1_ge *r, const secp256k1_ge_storage *a);

static void secp256k1_gej_cmov(secp256k1_gej *r, const secp256k1_gej *a, int flag);

static void secp256k1_ge_storage_cmov(secp256k1_ge_storage *r, const secp256k1_ge_storage *a, int flag);

static void secp256k1_gej_rescale(secp256k1_gej *r, const secp256k1_fe *b);

static int secp256k1_ge_is_in_correct_subgroup(const secp256k1_ge* ge);

static void secp256k1_ge_verify(const secp256k1_ge *a);
#define SECP256K1_GE_VERIFY(a) secp256k1_ge_verify(a)

static void secp256k1_gej_verify(const secp256k1_gej *a);
#define SECP256K1_GEJ_VERIFY(a) secp256k1_gej_verify(a)

#endif
