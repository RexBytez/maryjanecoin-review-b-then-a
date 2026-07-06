#ifndef MARYJANECOIN_MW_MODELS_BLOCK_H
#define MARYJANECOIN_MW_MODELS_BLOCK_H

#ifdef ENABLE_MWEB

#include "mw/models/tx_body.h"

#include <openssl/sha.h>

namespace mw {

class CMWState;

class CMWBlock
{
public:

    uint256 hashPrevMWBlock;

    CMWTransactionBody body;

    std::vector<Commitment> vCutThrough;

    int32_t nHeight;

    BlindingFactor offset;

    CMWBlock()
    {
        hashPrevMWBlock = 0;
        nHeight = 0;
    }

    bool IsNull() const
    {
        return body.IsNull() && hashPrevMWBlock == 0;
    }

    uint256 GetHash() const
    {
        uint256 hash;
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        SHA256_Update(&ctx, hashPrevMWBlock.begin(), 32);

        uint256 bodyHash = body.GetHash();
        SHA256_Update(&ctx, bodyHash.begin(), 32);

        SHA256_Update(&ctx, &nHeight, sizeof(nHeight));

        SHA256_Update(&ctx, offset.data, BLINDING_FACTOR_SIZE);

        uint32_t nCutThrough = (uint32_t)vCutThrough.size();
        SHA256_Update(&ctx, &nCutThrough, sizeof(nCutThrough));

        SHA256_Final((unsigned char*)&hash, &ctx);

        uint256 hash2;
        SHA256((unsigned char*)&hash, sizeof(hash), (unsigned char*)&hash2);
        return hash2;
    }

    bool Validate(const CMWState& state) const;

    size_t GetKernelCount() const { return body.vKernels.size(); }

    int64_t GetTotalFee() const { return body.GetTotalFee(); }

    int64_t GetTotalPegIn() const { return body.GetTotalPegIn(); }

    int64_t GetTotalPegOut() const { return body.GetTotalPegOut(); }

    IMPLEMENT_SERIALIZE(
        READWRITE(hashPrevMWBlock);
        READWRITE(body);
        READWRITE(vCutThrough);
        READWRITE(nHeight);
        READWRITE(offset);
    )
};

}

#endif
#endif
