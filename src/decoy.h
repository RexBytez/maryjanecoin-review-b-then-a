#ifndef MARYJANECOIN_DECOY_H
#define MARYJANECOIN_DECOY_H

#include <stdint.h>
#include "uint256.h"

static const int DECOY_PRIVACY_LEVEL_MIN     = 1;
static const int DECOY_PRIVACY_LEVEL_MAX     = 5;
static const int DECOY_PRIVACY_LEVEL_DEFAULT = 3;

static const int DECOY_THRESHOLD_BP[6] = { 0, 20, 50, 120, 300, 700 };
static const int DECOY_BP_SCALE = 10000;

inline int DecoyClampLevel(int level)
{
    if (level < DECOY_PRIVACY_LEVEL_MIN || level > DECOY_PRIVACY_LEVEL_MAX)
        return DECOY_PRIVACY_LEVEL_DEFAULT;
    return level;
}

inline uint64_t DecoyBeacon(const uint256& blockHash, uint64_t walletSalt)
{
    uint64_t a = blockHash.Get64(0) ^ walletSalt;
    uint64_t b = blockHash.Get64(1) + walletSalt;

    uint64_t x = a ^ (b * 0x9E3779B97F4A7C15ULL);
    x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27; x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

inline bool DecoyShouldInject(const uint256& blockHash, uint64_t walletSalt, int level)
{
    int lvl = DecoyClampLevel(level);
    uint64_t beacon = DecoyBeacon(blockHash, walletSalt);
    return (int)(beacon % DECOY_BP_SCALE) < DECOY_THRESHOLD_BP[lvl];
}

inline int DecoyBurstCount(const uint256& blockHash, uint64_t walletSalt, int level)
{
    if (!DecoyShouldInject(blockHash, walletSalt, level))
        return 0;
    int lvl = DecoyClampLevel(level);
    uint64_t b = DecoyBeacon(blockHash, walletSalt + 1);
    int maxBurst = (lvl >= 4) ? 3 : 2;
    return 1 + (int)(b % (uint64_t)maxBurst);
}

static const int DECOY_RECYCLE_BASE_BLOCKS = 1000;
inline bool DecoyShouldRecycle(int ageBlocks, int level)
{
    int lvl = DecoyClampLevel(level);
    int window = DECOY_RECYCLE_BASE_BLOCKS / lvl;
    return ageBlocks >= window;
}

#endif
