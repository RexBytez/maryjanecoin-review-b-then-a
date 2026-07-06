#ifndef SECP256K1_HSORT_H
#define SECP256K1_HSORT_H

#include <stddef.h>
#include <string.h>

static void secp256k1_hsort(void *ptr, size_t count, size_t size,
                            int (*cmp)(const void *, const void *, void *),
                            void *cmp_data);
#endif
