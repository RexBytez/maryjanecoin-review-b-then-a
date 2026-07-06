#ifndef SECP256K1_ECMULT_COMPUTE_TABLE_H
#define SECP256K1_ECMULT_COMPUTE_TABLE_H

static void secp256k1_ecmult_compute_table(secp256k1_ge_storage* table, int window_g, const secp256k1_gej* gen);

static void secp256k1_ecmult_compute_two_tables(secp256k1_ge_storage* table, secp256k1_ge_storage* table_128, int window_g, const secp256k1_ge* gen);

#endif
