#ifdef ENABLE_MWEB

#include "mw/crypto/schnorr.h"
#include "mw/crypto/pedersen.h"
#include "util.h"
#include "support/cleanse.h"

#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_extrakeys.h>

#include <openssl/sha.h>
#include <openssl/rand.h>

#include <cstring>
#include <cassert>

namespace mw {
namespace crypto {

static bool IsCanonicalCompressedPoint(secp256k1_context* ctx,
                                       const unsigned char* p33)
{

    bool fAllZero = true;
    for (size_t i = 0; i < 33; i++)
        if (p33[i] != 0) { fAllZero = false; break; }
    if (fAllZero)
        return false;

    secp256k1_pubkey pt;
    if (!secp256k1_ec_pubkey_parse(ctx, &pt, p33, 33))
        return false;

    unsigned char round[33];
    size_t len = sizeof(round);
    if (!secp256k1_ec_pubkey_serialize(ctx, round, &len, &pt, SECP256K1_EC_COMPRESSED))
        return false;
    if (len != 33 || memcmp(round, p33, 33) != 0)
        return false;

    return true;
}

Signature SchnorrSigner::Sign(const SecretKey& secretKey, const uint256& message)
{
    Signature result;

    PedersenContext& pedersen = PedersenContext::Get();
    secp256k1_context* ctx = pedersen.GetContext();

    secp256k1_keypair keypair;
    if (!secp256k1_keypair_create(ctx, &keypair, secretKey.data))
    {
        return result;
    }

    unsigned char sig64[64];
    if (!secp256k1_schnorrsig_sign32(ctx, sig64, (const unsigned char*)message.begin(), &keypair, NULL))
    {
        memory_cleanse(&keypair, sizeof(keypair));
        return result;
    }

    memcpy(result.data, sig64, 64);
    memory_cleanse(&keypair, sizeof(keypair));
    memory_cleanse(sig64, 64);

    return result;
}

Signature SchnorrSigner::SignExcess(const BlindingFactor& excessBlind, const uint256& message)
{
    SecretKey key(excessBlind.data);
    return Sign(key, message);
}

Signature SchnorrSigner::AggregateSign(
    const std::vector<SecretKey>& vKeys,
    const uint256& message)
{
    if (vKeys.empty())
        return Signature();

    if (vKeys.size() == 1)
        return Sign(vKeys[0], message);

    PedersenContext& pedersen = PedersenContext::Get();
    secp256k1_context* ctx = pedersen.GetContext();

    unsigned char aggKey[32];
    memcpy(aggKey, vKeys[0].data, 32);

    for (size_t i = 1; i < vKeys.size(); i++)
    {
        if (!secp256k1_ec_seckey_tweak_add(ctx, aggKey, vKeys[i].data))
        {
            memory_cleanse(aggKey, 32);
            return Signature();
        }
    }

    SecretKey aggregateKey(aggKey);
    memory_cleanse(aggKey, 32);

    return Sign(aggregateKey, message);
}

bool SchnorrSigner::GetPublicKey(const SecretKey& secretKey,
                                  unsigned char* pPubKey33)
{
    PedersenContext& pedersen = PedersenContext::Get();
    secp256k1_context* ctx = pedersen.GetContext();

    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, secretKey.data))
        return false;

    size_t outputLen = 33;
    return secp256k1_ec_pubkey_serialize(ctx, pPubKey33, &outputLen, &pubkey,
                                          SECP256K1_EC_COMPRESSED) == 1;
}

Commitment SchnorrSigner::GetExcessCommitment(const BlindingFactor& excessBlind)
{
    PedersenContext& pedersen = PedersenContext::Get();
    return pedersen.CommitBlind(excessBlind);
}

bool SchnorrVerifier::Verify(const unsigned char* pPubKey33,
                              const uint256& message,
                              const Signature& sig)
{
    PedersenContext& pedersen = PedersenContext::Get();
    secp256k1_context* ctx = pedersen.GetContext();

    if (!IsCanonicalCompressedPoint(ctx, pPubKey33))
        return false;

    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(ctx, &pubkey, pPubKey33, 33))
        return false;

    secp256k1_xonly_pubkey xonly;
    if (!secp256k1_xonly_pubkey_from_pubkey(ctx, &xonly, NULL, &pubkey))
        return false;

    return secp256k1_schnorrsig_verify(ctx, sig.data,
                                        (const unsigned char*)message.begin(),
                                        32, &xonly) == 1;
}

bool SchnorrVerifier::VerifyExcess(const Commitment& excess,
                                    const uint256& message,
                                    const Signature& sig)
{

    return Verify(excess.data, message, sig);
}

bool SchnorrVerifier::BatchVerify(
    const std::vector<const unsigned char*>& vPubKeys33,
    const std::vector<uint256>& vMessages,
    const std::vector<Signature>& vSigs)
{
    if (vPubKeys33.size() != vMessages.size() || vMessages.size() != vSigs.size())
        return false;

    if (vSigs.empty())
        return true;

    for (size_t i = 0; i < vSigs.size(); i++)
    {
        if (!Verify(vPubKeys33[i], vMessages[i], vSigs[i]))
            return false;
    }

    return true;
}

}
}

#endif
