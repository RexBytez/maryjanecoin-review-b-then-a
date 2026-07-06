#ifndef MARYJANECOIN_MW_CRYPTO_PEDERSEN_H
#define MARYJANECOIN_MW_CRYPTO_PEDERSEN_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"

#include <vector>
#include <stdint.h>

struct secp256k1_context_struct;
typedef struct secp256k1_context_struct secp256k1_context;

namespace mw {
namespace crypto {

class PedersenContext
{
private:
    secp256k1_context* pCtx;

    static const unsigned char GENERATOR_H[33];

    PedersenContext(const PedersenContext&);
    PedersenContext& operator=(const PedersenContext&);

public:
    PedersenContext();
    ~PedersenContext();

    static PedersenContext& Get();

    secp256k1_context* GetContext() { return pCtx; }

    Commitment Commit(int64_t nValue, const BlindingFactor& blind);

    bool VerifyCommitmentSum(
        const std::vector<Commitment>& vPositive,
        const std::vector<Commitment>& vNegative);

    BlindingFactor BlindSum(
        const std::vector<BlindingFactor>& vPositive,
        const std::vector<BlindingFactor>& vNegative);

    Commitment CommitBlind(const BlindingFactor& blind);

    BlindingFactor BlindAdd(const BlindingFactor& a, const BlindingFactor& b);

    BlindingFactor BlindNegate(const BlindingFactor& a);

    BlindingFactor GenerateBlindingFactor();

    Commitment SwitchCommit(int64_t nValue, const BlindingFactor& blind);

private:

    bool ScalarMultH(const unsigned char* scalar32, unsigned char* result33);
};

}
}

#endif
#endif
