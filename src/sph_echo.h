#ifndef SPH_ECHO_H__
#define SPH_ECHO_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include "sph_types.h"

#define SPH_SIZE_echo224   224

#define SPH_SIZE_echo256   256

#define SPH_SIZE_echo384   384

#define SPH_SIZE_echo512   512

typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[192];
	size_t ptr;
	union {
		sph_u32 Vs[4][4];
#if SPH_64
		sph_u64 Vb[4][2];
#endif
	} u;
	sph_u32 C0, C1, C2, C3;
#endif
} sph_echo_small_context;

typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[128];
	size_t ptr;
	union {
		sph_u32 Vs[8][4];
#if SPH_64
		sph_u64 Vb[8][2];
#endif
	} u;
	sph_u32 C0, C1, C2, C3;
#endif
} sph_echo_big_context;

typedef sph_echo_small_context sph_echo224_context;

typedef sph_echo_small_context sph_echo256_context;

typedef sph_echo_big_context sph_echo384_context;

typedef sph_echo_big_context sph_echo512_context;

void sph_echo224_init(void *cc);

void sph_echo224(void *cc, const void *data, size_t len);

void sph_echo224_close(void *cc, void *dst);

void sph_echo224_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_echo256_init(void *cc);

void sph_echo256(void *cc, const void *data, size_t len);

void sph_echo256_close(void *cc, void *dst);

void sph_echo256_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_echo384_init(void *cc);

void sph_echo384(void *cc, const void *data, size_t len);

void sph_echo384_close(void *cc, void *dst);

void sph_echo384_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

void sph_echo512_init(void *cc);

void sph_echo512(void *cc, const void *data, size_t len);

void sph_echo512_close(void *cc, void *dst);

void sph_echo512_addbits_and_close(
	void *cc, unsigned ub, unsigned n, void *dst);

#ifdef __cplusplus
}
#endif

#endif
