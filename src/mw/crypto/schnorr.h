#ifndef MARYJANECOIN_MW_CRYPTO_SCHNORR_H
#define MARYJANECOIN_MW_CRYPTO_SCHNORR_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"
#include "key.h"

#include <vector>
#include <stdint.h>

namespace mw {
namespace crypto {

class SchnorrSigner
{
public:

    static Signature Sign(const SecretKey& secretKey, const uint256& message);

    static Signature SignExcess(const BlindingFactor& excessBlind, const uint256& message);

    static Signature AggregateSign(
        const std::vector<SecretKey>& vKeys,
        const uint256& message);

    static bool GetPublicKey(const SecretKey& secretKey,
                             unsigned char* pPubKey33);

    static Commitment GetExcessCommitment(const BlindingFactor& excessBlind);
};

class SchnorrVerifier
{
public:

    static bool Verify(const unsigned char* pPubKey33,
                       const uint256& message,
                       const Signature& sig);

    static bool VerifyExcess(const Commitment& excess,
                             const uint256& message,
                             const Signature& sig);

    static bool BatchVerify(
        const std::vector<const unsigned char*>& vPubKeys33,
        const std::vector<uint256>& vMessages,
        const std::vector<Signature>& vSigs);
};

}
}

#endif
#endif
