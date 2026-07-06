#ifndef SPH_HAMSI_H__
#define SPH_HAMSI_H__

#include <stddef.h>
#include "sph_types.h"

#ifdef __cplusplus
extern "C"{
#endif

#define SPH_SIZE_hamsi224   224

#define SPH_SIZE_hamsi256   256

#define SPH_SIZE_hamsi384   384

#define SPH_SIZE_hamsi512   512

typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char partial[4];
	size_t partial_len;
	sph_u32 h[8];
#if SPH_64
	sph_u64 count;
#else
	sph_u32 count_high, count_low;
#endif
#endif
} sph_hamsi_small_context;

typedef sph_hamsi_small_context sph_hamsi224_context;

typedef sph_hamsi_small_context sph_hamsi256_context;

typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char partial[8];
	size_t partial_len;
	sph_u32 h[16];
#if SPH_64
	sph_u64 count;
#else
	sph_u32 count_high, count_low;
#endif
#endif
} sph_hamsi_big_context;

typedef sph_hamsi_big_context sph_hamsi384_context;

typedef sph_hamsi_big_context sph_hamsi512_context;

void sph_hamsi224_init(void *cc);

void sph_hamsi224(void *cc, const void *data, size_t len);

void sph_hamsi224_close(void *cc, void *dst);

void sph_hamsi224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_hamsi256_init(void *cc);

void sph_hamsi256(void *cc, const void *data, size_t len);

void sph_hamsi256_close(void *cc, void *dst);

void sph_hamsi256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_hamsi384_init(void *cc);

void sph_hamsi384(void *cc, const void *data, size_t len);

void sph_hamsi384_close(void *cc, void *dst);

void sph_hamsi384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_hamsi512_init(void *cc);

void sph_hamsi512(void *cc, const void *data, size_t len);

void sph_hamsi512_close(void *cc, void *dst);

void sph_hamsi512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

#ifdef __cplusplus
}
#endif

#endif
