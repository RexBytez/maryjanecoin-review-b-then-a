#ifndef SECP256K1_HSORT_IMPL_H
#define SECP256K1_HSORT_IMPL_H

#include "hsort.h"

static SECP256K1_INLINE size_t secp256k1_heap_child1(size_t i) {
    VERIFY_CHECK(i <= (SIZE_MAX - 1)/2);
    return 2*i + 1;
}

static SECP256K1_INLINE size_t secp256k1_heap_child2(size_t i) {
    VERIFY_CHECK(i <= SIZE_MAX/2 - 1);
    return secp256k1_heap_child1(i)+1;
}

static SECP256K1_INLINE void secp256k1_heap_swap64(unsigned char *a, unsigned char *b, size_t len) {
    unsigned char tmp[64];
    VERIFY_CHECK(len <= 64);
    memcpy(tmp, a, len);
    memmove(a, b, len);
    memcpy(b, tmp, len);
}

static SECP256K1_INLINE void secp256k1_heap_swap(unsigned char *arr, size_t i, size_t j, size_t stride) {
    unsigned char *a = arr + i*stride;
    unsigned char *b = arr + j*stride;
    size_t len = stride;
    while (64 < len) {
        secp256k1_heap_swap64(a + (len - 64), b + (len - 64), 64);
        len -= 64;
    }
    secp256k1_heap_swap64(a, b, len);
}

static SECP256K1_INLINE void secp256k1_heap_down(unsigned char *arr, size_t i, size_t heap_size, size_t stride,
                            int (*cmp)(const void *, const void *, void *), void *cmp_data) {
    while (i < heap_size/2) {
        VERIFY_CHECK(i <= SIZE_MAX/2 - 1);

        VERIFY_CHECK(secp256k1_heap_child1(i) < heap_size);

        if (secp256k1_heap_child2(i) < heap_size
                && 0 <= cmp(arr + secp256k1_heap_child2(i)*stride, arr + secp256k1_heap_child1(i)*stride, cmp_data)) {
            if (0 < cmp(arr + secp256k1_heap_child2(i)*stride, arr + i*stride, cmp_data)) {
                secp256k1_heap_swap(arr, i, secp256k1_heap_child2(i), stride);
                i = secp256k1_heap_child2(i);
            } else {

                return;
            }
        } else if (0 < cmp(arr + secp256k1_heap_child1(i)*stride, arr +         i*stride, cmp_data)) {
            secp256k1_heap_swap(arr, i, secp256k1_heap_child1(i), stride);
            i = secp256k1_heap_child1(i);
        } else {
            return;
        }
    }

}

static void secp256k1_hsort(void *ptr, size_t count, size_t size,
                            int (*cmp)(const void *, const void *, void *),
                            void *cmp_data) {
    size_t i;

    for (i = count/2; 0 < i; --i) {
        secp256k1_heap_down(ptr, i-1, count, size, cmp, cmp_data);
    }
    for (i = count; 1 < i; --i) {

        secp256k1_heap_swap(ptr, 0, i-1, size);

        secp256k1_heap_down(ptr, 0, i-1, size, cmp, cmp_data);
    }
}

#endif
