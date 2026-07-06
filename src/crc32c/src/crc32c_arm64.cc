#include "./crc32c_arm64.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "./crc32c_internal.h"
#ifdef CRC32C_HAVE_CONFIG_H
#include "crc32c/crc32c_config.h"
#endif

#if HAVE_ARM64_CRC32C

#include <arm_acle.h>
#include <arm_neon.h>

#define KBYTES 1032
#define SEGMENTBYTES 256

#define CRC32C32BYTES(P, IND)                                             \
  do {                                                                    \
    std::memcpy(&d64, (P) + SEGMENTBYTES * 1 + (IND) * 8, sizeof(d64));   \
    crc1 = __crc32cd(crc1, d64);                                          \
    std::memcpy(&d64, (P) + SEGMENTBYTES * 2 + (IND) * 8, sizeof(d64));   \
    crc2 = __crc32cd(crc2, d64);                                          \
    std::memcpy(&d64, (P) + SEGMENTBYTES * 3 + (IND) * 8, sizeof(d64));   \
    crc3 = __crc32cd(crc3, d64);                                          \
    std::memcpy(&d64, (P) + SEGMENTBYTES * 0 + (IND) * 8, sizeof(d64));   \
    crc0 = __crc32cd(crc0, d64);                                          \
  } while (0);

#define CRC32C256BYTES(P, IND)      \
  do {                              \
    CRC32C32BYTES((P), (IND)*8 + 0) \
    CRC32C32BYTES((P), (IND)*8 + 1) \
    CRC32C32BYTES((P), (IND)*8 + 2) \
    CRC32C32BYTES((P), (IND)*8 + 3) \
    CRC32C32BYTES((P), (IND)*8 + 4) \
    CRC32C32BYTES((P), (IND)*8 + 5) \
    CRC32C32BYTES((P), (IND)*8 + 6) \
    CRC32C32BYTES((P), (IND)*8 + 7) \
  } while (0);

#define CRC32C1024BYTES(P)   \
  do {                       \
    CRC32C256BYTES((P), 0)   \
    CRC32C256BYTES((P), 1)   \
    CRC32C256BYTES((P), 2)   \
    CRC32C256BYTES((P), 3)   \
    (P) += 4 * SEGMENTBYTES; \
  } while (0)

namespace crc32c {

uint32_t ExtendArm64(uint32_t crc, const uint8_t *data, size_t size) {
  int64_t length = size;
  uint32_t crc0, crc1, crc2, crc3;
  uint64_t t0, t1, t2;
  uint16_t d16;
  uint32_t d32;
  uint64_t d64;

  const poly64_t k0 = 0x8d96551c, k1 = 0xbd6f81f8, k2 = 0xdcb17aa4;

  crc = crc ^ kCRC32Xor;

  while (length >= KBYTES) {
    crc0 = crc;
    crc1 = 0;
    crc2 = 0;
    crc3 = 0;

    CRC32C1024BYTES(data);

    t2 = (uint64_t)vmull_p64(crc2, k2);
    t1 = (uint64_t)vmull_p64(crc1, k1);
    t0 = (uint64_t)vmull_p64(crc0, k0);
    std::memcpy(&d64, data, sizeof(d64));
    crc = __crc32cd(crc3, d64);
    data += sizeof(uint64_t);
    crc ^= __crc32cd(0, t2);
    crc ^= __crc32cd(0, t1);
    crc ^= __crc32cd(0, t0);

    length -= KBYTES;
  }

  while (length >= 8) {
    std::memcpy(&d64, data, sizeof(d64));
    crc = __crc32cd(crc, d64);
    data += 8;
    length -= 8;
  }

  if (length & 4) {
    std::memcpy(&d32, data, sizeof(d32));
    crc = __crc32cw(crc, d32);
    data += 4;
  }

  if (length & 2) {
    std::memcpy(&d16, data, sizeof(d16));
    crc = __crc32ch(crc, d16);
    data += 2;
  }

  if (length & 1) {
    crc = __crc32cb(crc, *data);
  }

  return crc ^ kCRC32Xor;
}

}

#endif
