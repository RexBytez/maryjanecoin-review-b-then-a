#ifndef MARYJANECOIN_MW_MODELS_INPUT_H
#define MARYJANECOIN_MW_MODELS_INPUT_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"
#include <openssl/sha.h>
#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_extrakeys.h>

namespace mw {

class CMWInput
{
public:

    Commitment commitment;

    unsigned char outputPubKey[PUBKEY_SIZE];

    Signature signature;

    CMWInput()
    {
        memset(outputPubKey, 0, PUBKEY_SIZE);
    }

    CMWInput(const Commitment& commitIn,
             const unsigned char* pPubKey,
             const Signature& sigIn)
        : commitment(commitIn), signature(sigIn)
    {
        memcpy(outputPubKey, pPubKey, PUBKEY_SIZE);
    }

    bool IsNull() const
    {
        return commitment.IsNull();
    }

    uint256 GetHash() const
    {
        uint256 hash;
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, commitment.data, COMMITMENT_SIZE);
        SHA256_Update(&ctx, outputPubKey, PUBKEY_SIZE);
        SHA256_Final((unsigned char*)hash.begin(), &ctx);
        return hash;
    }

    bool VerifySignature(const uint256& txHash) const
    {

        secp256k1_context* ctx256 = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
        if (!ctx256) return false;

        uint256 message;
        SHA256_CTX shactx;
        SHA256_Init(&shactx);
        SHA256_Update(&shactx, commitment.data, COMMITMENT_SIZE);
        SHA256_Update(&shactx, txHash.begin(), 32);
        SHA256_Final((unsigned char*)message.begin(), &shactx);

        secp256k1_xonly_pubkey xpub;
        bool fOk = secp256k1_xonly_pubkey_parse(ctx256, &xpub, outputPubKey + 1);

        if (fOk)
            fOk = secp256k1_schnorrsig_verify(ctx256, signature.data, (const unsigned char*)message.begin(), 32, &xpub);

        secp256k1_context_destroy(ctx256);
        return fOk;
    }

    friend bool operator==(const CMWInput& a, const CMWInput& b)
    {
        return a.commitment == b.commitment;
    }

    friend bool operator!=(const CMWInput& a, const CMWInput& b)
    {
        return !(a == b);
    }

    friend bool operator<(const CMWInput& a, const CMWInput& b)
    {
        return a.commitment < b.commitment;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(commitment);
        READWRITE(FLATDATA(outputPubKey));
        READWRITE(signature);
    )
};

}

#endif
#endif
