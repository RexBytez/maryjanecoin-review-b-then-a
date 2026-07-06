#ifndef SECP256K1_SCRATCH_H
#define SECP256K1_SCRATCH_H

typedef struct secp256k1_scratch_space_struct {

    unsigned char magic[8];

    void *data;

    size_t alloc_size;

    size_t max_size;
} secp256k1_scratch;

static secp256k1_scratch* secp256k1_scratch_create(const secp256k1_callback* error_callback, size_t max_size);

static void secp256k1_scratch_destroy(const secp256k1_callback* error_callback, secp256k1_scratch* scratch);

static size_t secp256k1_scratch_checkpoint(const secp256k1_callback* error_callback, const secp256k1_scratch* scratch);

static void secp256k1_scratch_apply_checkpoint(const secp256k1_callback* error_callback, secp256k1_scratch* scratch, size_t checkpoint);

static size_t secp256k1_scratch_max_allocation(const secp256k1_callback* error_callback, const secp256k1_scratch* scratch, size_t n_objects);

static void *secp256k1_scratch_alloc(const secp256k1_callback* error_callback, secp256k1_scratch* scratch, size_t n);

#endif
