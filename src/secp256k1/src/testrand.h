#ifndef SECP256K1_TESTRAND_H
#define SECP256K1_TESTRAND_H

#include "util.h"

SECP256K1_INLINE static void testrand_seed(const unsigned char *seed16);

SECP256K1_INLINE static uint32_t testrand32(void);

SECP256K1_INLINE static uint64_t testrand64(void);

SECP256K1_INLINE static uint64_t testrand_bits(int bits);

static uint32_t testrand_int(uint32_t range);

static void testrand256(unsigned char *b32);

static void testrand256_test(unsigned char *b32);

static void testrand_bytes_test(unsigned char *bytes, size_t len);

static void testrand_flip(unsigned char *b, size_t len);

static void testrand_init(const char* hexseed);

static void testrand_finish(void);

#endif
