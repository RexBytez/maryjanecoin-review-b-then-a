#ifndef MARYJANECOIN_MW_MODELS_KERNEL_H
#define MARYJANECOIN_MW_MODELS_KERNEL_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"
#include "script.h"

#include <openssl/sha.h>

namespace mw {

class CMWKernel
{
public:

    uint8_t nFeatures;

    int64_t nFee;

    int64_t nPegAmount;

    int32_t nLockHeight;

    Commitment excess;

    Signature signature;

    CScript pegoutScript;

    CMWKernel()
    {
        nFeatures = KERNEL_PLAIN;
        nFee = 0;
        nPegAmount = 0;
        nLockHeight = 0;
    }

    CMWKernel(uint8_t nFeaturesIn, int64_t nFeeIn, const Commitment& excessIn,
              const Signature& sigIn)
        : nFeatures(nFeaturesIn), nFee(nFeeIn), nPegAmount(0), nLockHeight(0),
          excess(excessIn), signature(sigIn)
    {
    }

    CMWKernel(const CMWKernel& other)
        : nFeatures(other.nFeatures), nFee(other.nFee), nPegAmount(other.nPegAmount),
          nLockHeight(other.nLockHeight), excess(other.excess), signature(other.signature),
          pegoutScript(other.pegoutScript.begin(), other.pegoutScript.end())
    {
    }

    CMWKernel& operator=(const CMWKernel& other)
    {
        if (this != &other)
        {
            nFeatures = other.nFeatures;
            nFee = other.nFee;
            nPegAmount = other.nPegAmount;
            nLockHeight = other.nLockHeight;
            excess = other.excess;
            signature = other.signature;
            pegoutScript = CScript(other.pegoutScript.begin(), other.pegoutScript.end());
        }
        return *this;
    }

    bool IsNull() const
    {
        return excess.IsNull();
    }

    bool IsPlain() const { return nFeatures == KERNEL_PLAIN; }
    bool IsPegIn() const { return nFeatures == KERNEL_PEGIN; }
    bool IsPegOut() const { return nFeatures == KERNEL_PEGOUT; }
    bool IsHeightLocked() const { return nFeatures == KERNEL_HEIGHT_LOCKED; }
    bool IsCoinbase() const { return nFeatures == KERNEL_COINBASE; }

    uint256 GetSignatureMessage() const
    {
        uint256 hash;
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        SHA256_Update(&ctx, &nFeatures, 1);

        unsigned char feeBytes[8];
        for (int i = 7; i >= 0; i--)
        {
            feeBytes[i] = (unsigned char)(nFee >> ((7 - i) * 8));
        }
        SHA256_Update(&ctx, feeBytes, 8);

        if (nFeatures == KERNEL_PEGIN || nFeatures == KERNEL_PEGOUT)
        {
            unsigned char pegBytes[8];
            for (int i = 7; i >= 0; i--)
            {
                pegBytes[i] = (unsigned char)(nPegAmount >> ((7 - i) * 8));
            }
            SHA256_Update(&ctx, pegBytes, 8);
        }

        if (nFeatures == KERNEL_HEIGHT_LOCKED)
        {
            unsigned char heightBytes[4];
            for (int i = 3; i >= 0; i--)
            {
                heightBytes[i] = (unsigned char)(nLockHeight >> ((3 - i) * 8));
            }
            SHA256_Update(&ctx, heightBytes, 4);
        }

        SHA256_Update(&ctx, excess.data, COMMITMENT_SIZE);

        if (nFeatures == KERNEL_PEGOUT && pegoutScript.size() > 0)
        {
            SHA256_Update(&ctx, pegoutScript.data(), pegoutScript.size());
        }

        SHA256_Final((unsigned char*)&hash, &ctx);

        uint256 hash2;
        SHA256((unsigned char*)&hash, sizeof(hash), (unsigned char*)&hash2);
        return hash2;
    }

    uint256 GetHash() const
    {
        return GetSignatureMessage();
    }

    bool Verify() const;

    friend bool operator==(const CMWKernel& a, const CMWKernel& b)
    {
        return a.excess == b.excess && a.nFeatures == b.nFeatures;
    }

    friend bool operator!=(const CMWKernel& a, const CMWKernel& b)
    {
        return !(a == b);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(nFeatures);
        READWRITE(nFee);
        if (nFeatures == KERNEL_PEGIN || nFeatures == KERNEL_PEGOUT)
        {
            READWRITE(nPegAmount);
        }
        if (nFeatures == KERNEL_HEIGHT_LOCKED)
        {
            READWRITE(nLockHeight);
        }
        READWRITE(excess);
        READWRITE(signature);
        if (nFeatures == KERNEL_PEGOUT)
        {
            READWRITE(pegoutScript);
        }
    )
};

}

#endif
#endif
