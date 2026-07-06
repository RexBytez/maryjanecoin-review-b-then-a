#ifndef MARYJANECOIN_MW_MODELS_OUTPUT_H
#define MARYJANECOIN_MW_MODELS_OUTPUT_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"

#include <openssl/sha.h>

namespace mw {

class CMWOutput
{
public:

    Commitment commitment;

    unsigned char senderPubKey[PUBKEY_SIZE];

    unsigned char receiverPubKey[PUBKEY_SIZE];

    RangeProof rangeProof;

    uint8_t nFeatures;

    CMWOutput()
    {
        memset(senderPubKey, 0, PUBKEY_SIZE);
        memset(receiverPubKey, 0, PUBKEY_SIZE);
        nFeatures = OUTPUT_STANDARD;
    }

    CMWOutput(const Commitment& commitIn,
              const unsigned char* pSenderKey,
              const unsigned char* pReceiverKey,
              const RangeProof& proofIn,
              uint8_t nFeaturesIn = OUTPUT_STANDARD)
        : commitment(commitIn), rangeProof(proofIn), nFeatures(nFeaturesIn)
    {
        memcpy(senderPubKey, pSenderKey, PUBKEY_SIZE);
        memcpy(receiverPubKey, pReceiverKey, PUBKEY_SIZE);
    }

    bool IsNull() const
    {
        return commitment.IsNull();
    }

    bool IsCoinbase() const
    {
        return nFeatures == OUTPUT_COINBASE;
    }

    uint256 GetOutputID() const
    {
        uint256 hash;
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, &nFeatures, 1);
        SHA256_Update(&ctx, commitment.data, COMMITMENT_SIZE);
        SHA256_Final((unsigned char*)&hash, &ctx);

        uint256 hash2;
        SHA256((unsigned char*)&hash, sizeof(hash), (unsigned char*)&hash2);
        return hash2;
    }

    bool Verify() const;

    uint256 GetHash() const
    {
        return GetOutputID();
    }

    friend bool operator==(const CMWOutput& a, const CMWOutput& b)
    {
        return a.commitment == b.commitment && a.nFeatures == b.nFeatures;
    }

    friend bool operator!=(const CMWOutput& a, const CMWOutput& b)
    {
        return !(a == b);
    }

    friend bool operator<(const CMWOutput& a, const CMWOutput& b)
    {
        return a.commitment < b.commitment;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(commitment);
        READWRITE(FLATDATA(senderPubKey));
        READWRITE(FLATDATA(receiverPubKey));
        READWRITE(rangeProof);
        READWRITE(nFeatures);
    )
};

}

#endif
#endif
