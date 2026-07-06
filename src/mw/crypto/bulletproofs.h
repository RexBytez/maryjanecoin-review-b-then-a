#ifndef MARYJANECOIN_MW_CRYPTO_BULLETPROOFS_H
#define MARYJANECOIN_MW_CRYPTO_BULLETPROOFS_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"
#include "mw/crypto/pedersen.h"

#include <vector>
#include <stdint.h>

namespace mw {
namespace crypto {

static const int BULLETPROOF_BITS         = 64;
static const size_t BULLETPROOF_MAX_SIZE  = 768;
static const int BULLETPROOF_MAX_BATCH    = 256;

class BulletproofProver
{
public:

    static RangeProof Prove(
        int64_t nValue,
        const BlindingFactor& blind,
        const uint256& nonce,
        const std::vector<unsigned char>& vExtraData = std::vector<unsigned char>());

    static RangeProof ProveWithMessage(
        int64_t nValue,
        const BlindingFactor& blind,
        const uint256& nonce,
        const std::vector<unsigned char>& vMessage);
};

class BulletproofVerifier
{
public:

    static bool Verify(
        const Commitment& commit,
        const RangeProof& proof);

    static bool BatchVerify(
        const std::vector<std::pair<Commitment, RangeProof>>& vProofs);

    static std::vector<unsigned char> ExtractExtraData(
        const RangeProof& proof);

    static bool RecoverMessage(
        const Commitment& commit,
        const RangeProof& proof,
        const uint256& nonce,
        int64_t& nValueOut,
        BlindingFactor& blindOut,
        std::vector<unsigned char>& vMessageOut);
};

}
}

#endif
#endif
