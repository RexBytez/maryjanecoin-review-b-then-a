#ifndef CRC32C_CRC32C_PREFETCH_H_
#define CRC32C_CRC32C_PREFETCH_H_

#include <cstddef>
#include <cstdint>

#ifdef CRC32C_HAVE_CONFIG_H
#include "crc32c/crc32c_config.h"
#endif

#if HAVE_MM_PREFETCH

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <xmmintrin.h>
#endif

#endif

namespace crc32c {

inline void RequestPrefetch(const uint8_t* address) {
#if HAVE_BUILTIN_PREFETCH

  __builtin_prefetch(reinterpret_cast<const char*>(address), 0 ,
                     0 );
#elif HAVE_MM_PREFETCH

  _mm_prefetch(reinterpret_cast<const char*>(address), _MM_HINT_NTA);
#else

  (void)address;
#endif
}

}

#endif
