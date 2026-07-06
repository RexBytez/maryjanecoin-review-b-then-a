#ifndef CRC32C_CRC32C_ARM_H_
#define CRC32C_CRC32C_ARM_H_

#include <cstddef>
#include <cstdint>

#ifdef CRC32C_HAVE_CONFIG_H
#include "crc32c/crc32c_config.h"
#endif

#if HAVE_ARM64_CRC32C

namespace crc32c {

uint32_t ExtendArm64(uint32_t crc, const uint8_t* data, size_t count);

}

#endif

#endif
