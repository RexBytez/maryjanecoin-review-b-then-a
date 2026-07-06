#ifdef ENABLE_MWEB

#include "mw/crypto/pedersen.h"
#include "util.h"
#include "support/cleanse.h"

#include <secp256k1.h>

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <cstring>
#include <cassert>

namespace mw {
namespace crypto {

const unsigned char PedersenContext::GENERATOR_H[33] = {
    0x02, 0x50, 0x92, 0x9b, 0x74, 0xc1, 0xa0, 0x49, 0x54, 0xb7, 0x8b, 0x4b,
    0x60, 0x35, 0xe9, 0x7a, 0x5e, 0x07, 0x8a, 0x5a, 0x0f, 0x28, 0xec, 0x96,
    0xd5, 0x47, 0xbf, 0xee, 0x9a, 0xce, 0x80, 0x3a, 0xc0
};

PedersenContext::PedersenContext()
{
    pCtx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    assert(pCtx != NULL);

    unsigned char seed[32];
    RAND_bytes(seed, 32);
    secp256k1_context_randomize(pCtx, seed);
    memory_cleanse(seed, 32);
}

PedersenContext::~PedersenContext()
{
    if (pCtx)
    {
        secp256k1_context_destroy(pCtx);
        pCtx = NULL;
    }
}

PedersenContext& PedersenContext::Get()
{
    static PedersenContext instance;
    return instance;
}

Commitment PedersenContext::Commit(int64_t nValue, const BlindingFactor& blind)
{
    Commitment result;

    secp256k1_pubkey pubkey_rG;
    if (!secp256k1_ec_pubkey_create(pCtx, &pubkey_rG, blind.data))
    {

        return result;
    }

    unsigned char valueScalar[32];
    memset(valueScalar, 0, 32);

    uint64_t absValue = (nValue >= 0) ? (uint64_t)nValue : 0;
    for (int i = 31; i >= 0 && absValue > 0; i--)
    {
        valueScalar[i] = (unsigned char)(absValue & 0xFF);
        absValue >>= 8;
    }

    secp256k1_pubkey pubkey_H;
    if (!secp256k1_ec_pubkey_parse(pCtx, &pubkey_H, GENERATOR_H, 33))
    {
        memory_cleanse(valueScalar, 32);
        return result;
    }

    secp256k1_pubkey pubkey_vH = pubkey_H;

    bool fValueZero = true;
    for (int i = 0; i < 32; i++)
    {
        if (valueScalar[i] != 0) { fValueZero = false; break; }
    }

    if (fValueZero)
    {

        unsigned char output[33];
        size_t outputLen = 33;
        secp256k1_ec_pubkey_serialize(pCtx, output, &outputLen, &pubkey_rG, SECP256K1_EC_COMPRESSED);
        memcpy(result.data, output, 33);
        memory_cleanse(valueScalar, 32);
        return result;
    }

    if (!secp256k1_ec_pubkey_tweak_mul(pCtx, &pubkey_vH, valueScalar))
    {
        memory_cleanse(valueScalar, 32);
        return result;
    }
    memory_cleanse(valueScalar, 32);

    const secp256k1_pubkey* pubkeys[2] = { &pubkey_rG, &pubkey_vH };
    secp256k1_pubkey pubkey_combined;
    if (!secp256k1_ec_pubkey_combine(pCtx, &pubkey_combined, pubkeys, 2))
    {
        return result;
    }

    unsigned char output[33];
    size_t outputLen = 33;
    secp256k1_ec_pubkey_serialize(pCtx, output, &outputLen, &pubkey_combined, SECP256K1_EC_COMPRESSED);
    memcpy(result.data, output, 33);

    return result;
}

bool PedersenContext::VerifyCommitmentSum(
    const std::vector<Commitment>& vPositive,
    const std::vector<Commitment>& vNegative)
{
    if (vPositive.empty() && vNegative.empty())
        return true;

    std::vector<secp256k1_pubkey> posKeys(vPositive.size());
    std::vector<const secp256k1_pubkey*> posPtrs(vPositive.size());
    for (size_t i = 0; i < vPositive.size(); i++)
    {
        if (!secp256k1_ec_pubkey_parse(pCtx, &posKeys[i], vPositive[i].data, COMMITMENT_SIZE))
            return false;
        posPtrs[i] = &posKeys[i];
    }

    std::vector<secp256k1_pubkey> negKeys(vNegative.size());
    std::vector<const secp256k1_pubkey*> negPtrs(vNegative.size());
    for (size_t i = 0; i < vNegative.size(); i++)
    {
        if (!secp256k1_ec_pubkey_parse(pCtx, &negKeys[i], vNegative[i].data, COMMITMENT_SIZE))
            return false;
        negPtrs[i] = &negKeys[i];
    }

    for (size_t i = 0; i < negKeys.size(); i++)
    {
        secp256k1_ec_pubkey_negate(pCtx, &negKeys[i]);
    }

    std::vector<const secp256k1_pubkey*> allPtrs;
    allPtrs.insert(allPtrs.end(), posPtrs.begin(), posPtrs.end());
    allPtrs.insert(allPtrs.end(), negPtrs.begin(), negPtrs.end());

    if (allPtrs.empty())
        return true;

    secp256k1_pubkey combined;
    if (!secp256k1_ec_pubkey_combine(pCtx, &combined, allPtrs.data(), allPtrs.size()))
    {

        return true;
    }

    return false;
}

BlindingFactor PedersenContext::BlindSum(
    const std::vector<BlindingFactor>& vPositive,
    const std::vector<BlindingFactor>& vNegative)
{
    BlindingFactor result;

    if (vPositive.empty() && vNegative.empty())
        return result;

    unsigned char sum[32];
    memset(sum, 0, 32);

    for (size_t i = 0; i < vPositive.size(); i++)
    {
        if (i == 0)
        {
            memcpy(sum, vPositive[i].data, 32);
        }
        else
        {

            unsigned char temp[32];
            memcpy(temp, sum, 32);
            if (!secp256k1_ec_seckey_tweak_add(pCtx, temp, vPositive[i].data))
            {
                memory_cleanse(sum, 32);
                return result;
            }
            memcpy(sum, temp, 32);
        }
    }

    for (size_t i = 0; i < vNegative.size(); i++)
    {

        unsigned char negated[32];
        memcpy(negated, vNegative[i].data, 32);
        if (!secp256k1_ec_seckey_negate(pCtx, negated))
        {
            memory_cleanse(sum, 32);
            memory_cleanse(negated, 32);
            return result;
        }

        if (vPositive.empty() && i == 0)
        {
            memcpy(sum, negated, 32);
        }
        else
        {
            unsigned char temp[32];
            memcpy(temp, sum, 32);
            if (!secp256k1_ec_seckey_tweak_add(pCtx, temp, negated))
            {
                memory_cleanse(sum, 32);
                memory_cleanse(negated, 32);
                return result;
            }
            memcpy(sum, temp, 32);
        }
        memory_cleanse(negated, 32);
    }

    memcpy(result.data, sum, 32);
    memory_cleanse(sum, 32);
    return result;
}

Commitment PedersenContext::CommitBlind(const BlindingFactor& blind)
{
    return Commit(0, blind);
}

BlindingFactor PedersenContext::BlindAdd(const BlindingFactor& a, const BlindingFactor& b)
{
    BlindingFactor result;
    unsigned char sum[32];
    memcpy(sum, a.data, 32);
    if (!secp256k1_ec_seckey_tweak_add(pCtx, sum, b.data))
    {
        memory_cleanse(sum, 32);
        return result;
    }
    memcpy(result.data, sum, 32);
    memory_cleanse(sum, 32);
    return result;
}

BlindingFactor PedersenContext::BlindNegate(const BlindingFactor& a)
{
    BlindingFactor result;
    memcpy(result.data, a.data, 32);
    secp256k1_ec_seckey_negate(pCtx, result.data);
    return result;
}

BlindingFactor PedersenContext::GenerateBlindingFactor()
{
    BlindingFactor result;

    for (int attempts = 0; attempts < 100; attempts++)
    {
        RAND_bytes(result.data, BLINDING_FACTOR_SIZE);

        if (secp256k1_ec_seckey_verify(pCtx, result.data))
            return result;
    }

    memory_cleanse(result.data, BLINDING_FACTOR_SIZE);
    return result;
}

Commitment PedersenContext::SwitchCommit(int64_t nValue, const BlindingFactor& blind)
{
    return Commit(nValue, blind);
}

}

uint256 Commitment::GetHash() const
{
    uint256 hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, COMMITMENT_SIZE);
    SHA256_Final((unsigned char*)&hash, &ctx);

    uint256 hash2;
    SHA256((unsigned char*)&hash, sizeof(hash), (unsigned char*)&hash2);
    return hash2;
}

}

#endif
