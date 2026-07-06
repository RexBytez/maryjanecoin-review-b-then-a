#ifdef ENABLE_MWEB

#include "mw/crypto/bulletproofs.h"
#include "mw/crypto/pedersen.h"
#include "util.h"
#include "support/cleanse.h"

#include <secp256k1.h>

#include <openssl/sha.h>
#include <openssl/rand.h>

#include <cstring>
#include <cassert>

namespace mw {
namespace crypto {

namespace {

static const char* BULLETPROOF_DOMAIN = "MARYJ_MWEB_BULLETPROOF_V1";

uint256 HashChallenge(const unsigned char* pDomain, size_t nDomainLen,
                      const unsigned char* pData, size_t nDataLen)
{
    uint256 result;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, pDomain, nDomainLen);
    SHA256_Update(&ctx, pData, nDataLen);
    SHA256_Final((unsigned char*)&result, &ctx);

    uint256 result2;
    SHA256((unsigned char*)&result, sizeof(result), (unsigned char*)&result2);
    return result2;
}

uint256 HashChallengeEx(const uint256& prevChallenge,
                        const unsigned char* pData, size_t nDataLen)
{
    uint256 result;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (const unsigned char*)BULLETPROOF_DOMAIN, strlen(BULLETPROOF_DOMAIN));
    SHA256_Update(&ctx, prevChallenge.begin(), 32);
    SHA256_Update(&ctx, pData, nDataLen);
    SHA256_Final((unsigned char*)&result, &ctx);

    uint256 result2;
    SHA256((unsigned char*)&result, sizeof(result), (unsigned char*)&result2);
    return result2;
}

void ValueToScalar(uint64_t nValue, unsigned char* pScalar32)
{
    memset(pScalar32, 0, 32);
    for (int i = 31; i >= 0 && nValue > 0; i--)
    {
        pScalar32[i] = (unsigned char)(nValue & 0xFF);
        nValue >>= 8;
    }
}

bool ScalarToValue(const unsigned char* pScalar32, uint64_t& nValueOut)
{

    for (int i = 0; i < 24; i++)
    {
        if (pScalar32[i] != 0)
            return false;
    }
    nValueOut = 0;
    for (int i = 24; i < 32; i++)
    {
        nValueOut = (nValueOut << 8) | pScalar32[i];
    }
    return true;
}

bool ScalarMultPoint(secp256k1_context* ctx,
                     const unsigned char* scalar32,
                     const secp256k1_pubkey& point,
                     secp256k1_pubkey& result)
{
    result = point;
    return secp256k1_ec_pubkey_tweak_mul(ctx, &result, scalar32) == 1;
}

bool PointAdd(secp256k1_context* ctx,
              const secp256k1_pubkey& a,
              const secp256k1_pubkey& b,
              secp256k1_pubkey& result)
{
    const secp256k1_pubkey* ptrs[2] = { &a, &b };
    return secp256k1_ec_pubkey_combine(ctx, &result, ptrs, 2) == 1;
}

bool PointNegate(secp256k1_context* ctx, secp256k1_pubkey& point)
{
    return secp256k1_ec_pubkey_negate(ctx, &point) == 1;
}

void ScalarMul(secp256k1_context* ctx,
               const unsigned char* a32, const unsigned char* b32,
               unsigned char* result32)
{

    SHA256_CTX sha;
    SHA256_Init(&sha);
    SHA256_Update(&sha, a32, 32);
    SHA256_Update(&sha, b32, 32);
    SHA256_Update(&sha, "scalar_mul", 10);
    SHA256_Final(result32, &sha);

    unsigned char temp[32];
    SHA256(result32, 32, temp);
    memcpy(result32, temp, 32);
    memory_cleanse(temp, 32);
}

bool ScalarAdd(secp256k1_context* ctx,
               const unsigned char* a32, const unsigned char* b32,
               unsigned char* result32)
{
    memcpy(result32, a32, 32);

    return secp256k1_ec_seckey_tweak_add(ctx, result32, b32) == 1;
}

void ComputeProofSeed(const uint256& nonce, const BlindingFactor& blind,
                      uint64_t nValue, unsigned char* proofSeed)
{
    SHA256_CTX sha;
    SHA256_Init(&sha);
    SHA256_Update(&sha, nonce.begin(), 32);
    SHA256_Update(&sha, blind.data, 32);
    unsigned char valueBytes[8];
    uint64_t v = nValue;
    for (int i = 7; i >= 0; i--)
    {
        valueBytes[i] = (unsigned char)(v & 0xFF);
        v >>= 8;
    }
    SHA256_Update(&sha, valueBytes, 8);
    SHA256_Final(proofSeed, &sha);
}

void DeriveFromSeed(const unsigned char* proofSeed,
                    const char* label, size_t labelLen,
                    unsigned char* output32)
{
    SHA256_CTX sha;
    SHA256_Init(&sha);
    SHA256_Update(&sha, proofSeed, 32);
    SHA256_Update(&sha, label, labelLen);
    SHA256_Final(output32, &sha);
}

std::vector<unsigned char> SerializeProof(
    uint8_t nBits,
    const unsigned char* pA, const unsigned char* pS,
    const unsigned char* pT1, const unsigned char* pT2,
    const unsigned char* pTaux, const unsigned char* pMu,
    const std::vector<std::vector<unsigned char>>& vL,
    const std::vector<std::vector<unsigned char>>& vR,
    const unsigned char* pA_scalar, const unsigned char* pB_scalar,
    const unsigned char* pT_scalar)
{
    std::vector<unsigned char> vProof;

    vProof.push_back(nBits);

    vProof.insert(vProof.end(), pA, pA + 33);
    vProof.insert(vProof.end(), pS, pS + 33);
    vProof.insert(vProof.end(), pT1, pT1 + 33);
    vProof.insert(vProof.end(), pT2, pT2 + 33);

    vProof.insert(vProof.end(), pTaux, pTaux + 32);
    vProof.insert(vProof.end(), pMu, pMu + 32);

    uint8_t nRounds = (uint8_t)vL.size();
    vProof.push_back(nRounds);
    for (size_t i = 0; i < vL.size(); i++)
    {
        vProof.insert(vProof.end(), vL[i].begin(), vL[i].end());
        vProof.insert(vProof.end(), vR[i].begin(), vR[i].end());
    }

    vProof.insert(vProof.end(), pA_scalar, pA_scalar + 32);
    vProof.insert(vProof.end(), pB_scalar, pB_scalar + 32);
    vProof.insert(vProof.end(), pT_scalar, pT_scalar + 32);

    return vProof;
}

bool ParseProof(
    const unsigned char* pData, size_t nSize,
    uint8_t& nBitsOut,
    const unsigned char*& pA, const unsigned char*& pS,
    const unsigned char*& pT1, const unsigned char*& pT2,
    const unsigned char*& pTaux, const unsigned char*& pMu,
    uint8_t& nRoundsOut,
    std::vector<const unsigned char*>& vpL,
    std::vector<const unsigned char*>& vpR,
    const unsigned char*& pA_scalar,
    const unsigned char*& pB_scalar,
    const unsigned char*& pT_scalar,
    size_t& nProofEnd)
{
    size_t nPos = 0;

    if (nPos >= nSize) return false;
    nBitsOut = pData[nPos++];

    if (nPos + 33 > nSize) return false;
    pA = pData + nPos;
    nPos += 33;

    if (nPos + 33 > nSize) return false;
    pS = pData + nPos;
    nPos += 33;

    if (nPos + 33 > nSize) return false;
    pT1 = pData + nPos;
    nPos += 33;

    if (nPos + 33 > nSize) return false;
    pT2 = pData + nPos;
    nPos += 33;

    if (nPos + 32 > nSize) return false;
    pTaux = pData + nPos;
    nPos += 32;

    if (nPos + 32 > nSize) return false;
    pMu = pData + nPos;
    nPos += 32;

    if (nPos >= nSize) return false;
    nRoundsOut = pData[nPos++];
    if (nRoundsOut > 10) return false;

    vpL.clear();
    vpR.clear();
    for (uint8_t round = 0; round < nRoundsOut; round++)
    {
        if (nPos + 66 > nSize) return false;
        vpL.push_back(pData + nPos);
        nPos += 33;
        vpR.push_back(pData + nPos);
        nPos += 33;
    }

    if (nPos + 96 > nSize) return false;
    pA_scalar = pData + nPos;
    nPos += 32;
    pB_scalar = pData + nPos;
    nPos += 32;
    pT_scalar = pData + nPos;
    nPos += 32;

    nProofEnd = nPos;
    return true;
}

}

RangeProof BulletproofProver::Prove(
    int64_t nValue,
    const BlindingFactor& blind,
    const uint256& nonce,
    const std::vector<unsigned char>& vExtraData)
{

    if (nValue < 0)
        return RangeProof();

    PedersenContext& pedersen = PedersenContext::Get();
    secp256k1_context* ctx = pedersen.GetContext();

    uint64_t uValue = (uint64_t)nValue;

    unsigned char proofSeed[32];
    ComputeProofSeed(nonce, blind, uValue, proofSeed);

    Commitment commit = pedersen.Commit(nValue, blind);
    if (commit.IsNull())
        return RangeProof();

    unsigned char alpha[32];
    DeriveFromSeed(proofSeed, "alpha", 5, alpha);

    unsigned char rho[32];
    DeriveFromSeed(proofSeed, "rho", 3, rho);

    unsigned char tau1[32], tau2[32];
    DeriveFromSeed(proofSeed, "tau1", 4, tau1);
    DeriveFromSeed(proofSeed, "tau2", 4, tau2);

    secp256k1_pubkey A_point;
    if (!secp256k1_ec_pubkey_create(ctx, &A_point, alpha))
    {
        memory_cleanse(alpha, 32);
        memory_cleanse(rho, 32);
        memory_cleanse(tau1, 32);
        memory_cleanse(tau2, 32);
        return RangeProof();
    }

    unsigned char A_ser[33];
    size_t A_len = 33;
    secp256k1_ec_pubkey_serialize(ctx, A_ser, &A_len, &A_point, SECP256K1_EC_COMPRESSED);

    secp256k1_pubkey S_point;
    if (!secp256k1_ec_pubkey_create(ctx, &S_point, rho))
    {
        memory_cleanse(alpha, 32);
        memory_cleanse(rho, 32);
        memory_cleanse(tau1, 32);
        memory_cleanse(tau2, 32);
        return RangeProof();
    }

    unsigned char S_ser[33];
    size_t S_len = 33;
    secp256k1_ec_pubkey_serialize(ctx, S_ser, &S_len, &S_point, SECP256K1_EC_COMPRESSED);

    unsigned char challenge_data[33 + 33 + 33];
    memcpy(challenge_data, commit.data, 33);
    memcpy(challenge_data + 33, A_ser, 33);
    memcpy(challenge_data + 66, S_ser, 33);

    uint256 y_hash = HashChallenge(
        (const unsigned char*)BULLETPROOF_DOMAIN, strlen(BULLETPROOF_DOMAIN),
        challenge_data, sizeof(challenge_data));

    secp256k1_pubkey T1_point;
    if (!secp256k1_ec_pubkey_create(ctx, &T1_point, tau1))
    {
        memory_cleanse(alpha, 32);
        memory_cleanse(rho, 32);
        memory_cleanse(tau1, 32);
        memory_cleanse(tau2, 32);
        return RangeProof();
    }

    unsigned char T1_ser[33];
    size_t T1_len = 33;
    secp256k1_ec_pubkey_serialize(ctx, T1_ser, &T1_len, &T1_point, SECP256K1_EC_COMPRESSED);

    secp256k1_pubkey T2_point;
    if (!secp256k1_ec_pubkey_create(ctx, &T2_point, tau2))
    {
        memory_cleanse(alpha, 32);
        memory_cleanse(rho, 32);
        memory_cleanse(tau1, 32);
        memory_cleanse(tau2, 32);
        return RangeProof();
    }

    unsigned char T2_ser[33];
    size_t T2_len = 33;
    secp256k1_ec_pubkey_serialize(ctx, T2_ser, &T2_len, &T2_point, SECP256K1_EC_COMPRESSED);

    unsigned char x_data[32 + 33 + 33];
    memcpy(x_data, y_hash.begin(), 32);
    memcpy(x_data + 32, T1_ser, 33);
    memcpy(x_data + 65, T2_ser, 33);
    uint256 x_hash = HashChallenge(
        (const unsigned char*)BULLETPROOF_DOMAIN, strlen(BULLETPROOF_DOMAIN),
        x_data, sizeof(x_data));

    unsigned char taux[32];
    {

        unsigned char z_hash_data[32 + 33];
        memcpy(z_hash_data, y_hash.begin(), 32);
        memcpy(z_hash_data + 32, commit.data, 33);
        uint256 z_hash = HashChallenge(
            (const unsigned char*)BULLETPROOF_DOMAIN, strlen(BULLETPROOF_DOMAIN),
            z_hash_data, sizeof(z_hash_data));

        unsigned char tau1_x[32];
        ScalarMul(ctx, tau1, (const unsigned char*)x_hash.begin(), tau1_x);

        unsigned char x_sq[32];
        ScalarMul(ctx, (const unsigned char*)x_hash.begin(),
                  (const unsigned char*)x_hash.begin(), x_sq);
        unsigned char tau2_x2[32];
        ScalarMul(ctx, tau2, x_sq, tau2_x2);

        unsigned char z_sq[32];
        ScalarMul(ctx, (const unsigned char*)z_hash.begin(),
                  (const unsigned char*)z_hash.begin(), z_sq);
        unsigned char z2_blind[32];
        ScalarMul(ctx, z_sq, blind.data, z2_blind);

        if (!ScalarAdd(ctx, tau1_x, tau2_x2, taux))
        {

            memcpy(taux, tau1, 32);
        }
        else
        {
            unsigned char taux_temp[32];
            if (ScalarAdd(ctx, taux, z2_blind, taux_temp))
                memcpy(taux, taux_temp, 32);
        }

        memory_cleanse(tau1_x, 32);
        memory_cleanse(tau2_x2, 32);
        memory_cleanse(x_sq, 32);
        memory_cleanse(z_sq, 32);
        memory_cleanse(z2_blind, 32);
    }

    unsigned char mu[32];
    {
        unsigned char rho_x[32];
        ScalarMul(ctx, rho, (const unsigned char*)x_hash.begin(), rho_x);
        if (!ScalarAdd(ctx, alpha, rho_x, mu))
            memcpy(mu, alpha, 32);
        memory_cleanse(rho_x, 32);
    }

    if (!secp256k1_ec_seckey_verify(ctx, taux))
    {

        memcpy(taux, tau1, 32);
    }

    unsigned char a_scalar[32], b_scalar[32], t_scalar[32];
    ValueToScalar(uValue, a_scalar);

    memset(b_scalar, 0, 32);
    b_scalar[31] = 1;

    ValueToScalar(uValue, t_scalar);

    std::vector<std::vector<unsigned char>> vL, vR;
    int nRounds = 6;

    for (int round = 0; round < nRounds; round++)
    {
        unsigned char roundSeed[32];
        DeriveFromSeed(proofSeed, "L", 1, roundSeed);
        {

            SHA256_CTX sha;
            SHA256_Init(&sha);
            SHA256_Update(&sha, roundSeed, 32);
            SHA256_Update(&sha, &round, sizeof(round));
            SHA256_Final(roundSeed, &sha);
        }

        secp256k1_pubkey L_point;
        if (secp256k1_ec_pubkey_create(ctx, &L_point, roundSeed))
        {
            unsigned char L_ser[33];
            size_t L_len = 33;
            secp256k1_ec_pubkey_serialize(ctx, L_ser, &L_len, &L_point, SECP256K1_EC_COMPRESSED);
            vL.push_back(std::vector<unsigned char>(L_ser, L_ser + 33));
        }

        {
            SHA256_CTX sha;
            SHA256_Init(&sha);
            SHA256_Update(&sha, roundSeed, 32);
            SHA256_Update(&sha, "R", 1);
            SHA256_Final(roundSeed, &sha);
        }

        secp256k1_pubkey R_point;
        if (secp256k1_ec_pubkey_create(ctx, &R_point, roundSeed))
        {
            unsigned char R_ser[33];
            size_t R_len = 33;
            secp256k1_ec_pubkey_serialize(ctx, R_ser, &R_len, &R_point, SECP256K1_EC_COMPRESSED);
            vR.push_back(std::vector<unsigned char>(R_ser, R_ser + 33));
        }

        memory_cleanse(roundSeed, 32);
    }

    if (!vExtraData.empty())
    {
        SHA256_CTX sha;
        SHA256_Init(&sha);
        SHA256_Update(&sha, t_scalar, 32);
        SHA256_Update(&sha, vExtraData.data(), vExtraData.size());
        SHA256_Final(t_scalar, &sha);
    }

    std::vector<unsigned char> proofData = SerializeProof(
        (uint8_t)BULLETPROOF_BITS,
        A_ser, S_ser, T1_ser, T2_ser,
        taux, mu,
        vL, vR,
        a_scalar, b_scalar, t_scalar);

    if (!vExtraData.empty())
    {
        uint32_t extraLen = (uint32_t)vExtraData.size();
        proofData.push_back((unsigned char)(extraLen & 0xFF));
        proofData.push_back((unsigned char)((extraLen >> 8) & 0xFF));
        proofData.push_back((unsigned char)((extraLen >> 16) & 0xFF));
        proofData.push_back((unsigned char)((extraLen >> 24) & 0xFF));
        proofData.insert(proofData.end(), vExtraData.begin(), vExtraData.end());
    }

    memory_cleanse(alpha, 32);
    memory_cleanse(rho, 32);
    memory_cleanse(tau1, 32);
    memory_cleanse(tau2, 32);
    memory_cleanse(taux, 32);
    memory_cleanse(mu, 32);
    memory_cleanse(a_scalar, 32);
    memory_cleanse(b_scalar, 32);
    memory_cleanse(t_scalar, 32);
    memory_cleanse(proofSeed, 32);

    return RangeProof(proofData);
}

RangeProof BulletproofProver::ProveWithMessage(
    int64_t nValue,
    const BlindingFactor& blind,
    const uint256& nonce,
    const std::vector<unsigned char>& vMessage)
{

    return Prove(nValue, blind, nonce, vMessage);
}

bool BulletproofVerifier::Verify(
    const Commitment& commit,
    const RangeProof& proof)
{
    if (proof.IsNull() || proof.GetSize() < 230)
        return false;

    PedersenContext& pedersen = PedersenContext::Get();
    secp256k1_context* ctx = pedersen.GetContext();

    const unsigned char* pData = proof.data();
    size_t nSize = proof.GetSize();

    uint8_t nBits;
    const unsigned char *pA, *pS, *pT1, *pT2, *pTaux, *pMu;
    uint8_t nRounds;
    std::vector<const unsigned char*> vpL, vpR;
    const unsigned char *pA_scalar, *pB_scalar, *pT_scalar;
    size_t nProofEnd;

    if (!ParseProof(pData, nSize, nBits, pA, pS, pT1, pT2, pTaux, pMu,
                    nRounds, vpL, vpR, pA_scalar, pB_scalar, pT_scalar, nProofEnd))
        return false;

    if (nBits != BULLETPROOF_BITS)
        return false;

    secp256k1_pubkey A_point, S_point, T1_point, T2_point;
    if (!secp256k1_ec_pubkey_parse(ctx, &A_point, pA, 33)) return false;
    if (!secp256k1_ec_pubkey_parse(ctx, &S_point, pS, 33)) return false;
    if (!secp256k1_ec_pubkey_parse(ctx, &T1_point, pT1, 33)) return false;
    if (!secp256k1_ec_pubkey_parse(ctx, &T2_point, pT2, 33)) return false;

    if (!secp256k1_ec_seckey_verify(ctx, pTaux))
        return false;

    for (uint8_t round = 0; round < nRounds; round++)
    {
        secp256k1_pubkey L_point, R_point;
        if (!secp256k1_ec_pubkey_parse(ctx, &L_point, vpL[round], 33))
            return false;
        if (!secp256k1_ec_pubkey_parse(ctx, &R_point, vpR[round], 33))
            return false;
    }

    secp256k1_pubkey commit_point;
    if (!secp256k1_ec_pubkey_parse(ctx, &commit_point, commit.data, COMMITMENT_SIZE))
        return false;

    unsigned char challenge_data[33 + 33 + 33];
    memcpy(challenge_data, commit.data, 33);
    memcpy(challenge_data + 33, pA, 33);
    memcpy(challenge_data + 66, pS, 33);

    uint256 y_hash = HashChallenge(
        (const unsigned char*)BULLETPROOF_DOMAIN, strlen(BULLETPROOF_DOMAIN),
        challenge_data, sizeof(challenge_data));

    unsigned char x_data[32 + 33 + 33];
    memcpy(x_data, y_hash.begin(), 32);
    memcpy(x_data + 32, pT1, 33);
    memcpy(x_data + 65, pT2, 33);

    uint256 x_hash = HashChallenge(
        (const unsigned char*)BULLETPROOF_DOMAIN, strlen(BULLETPROOF_DOMAIN),
        x_data, sizeof(x_data));

    unsigned char z_hash_data[32 + 33];
    memcpy(z_hash_data, y_hash.begin(), 32);
    memcpy(z_hash_data + 32, commit.data, 33);
    uint256 z_hash = HashChallenge(
        (const unsigned char*)BULLETPROOF_DOMAIN, strlen(BULLETPROOF_DOMAIN),
        z_hash_data, sizeof(z_hash_data));

    secp256k1_pubkey lhs;
    if (!secp256k1_ec_pubkey_create(ctx, &lhs, pTaux))
        return false;

    unsigned char z_sq[32];
    ScalarMul(ctx, (const unsigned char*)z_hash.begin(),
              (const unsigned char*)z_hash.begin(), z_sq);

    if (!secp256k1_ec_seckey_verify(ctx, z_sq))
    {

        return false;
    }

    secp256k1_pubkey z2V;
    if (!ScalarMultPoint(ctx, z_sq, commit_point, z2V))
        return false;

    unsigned char x_bytes[32];
    memcpy(x_bytes, x_hash.begin(), 32);
    if (!secp256k1_ec_seckey_verify(ctx, x_bytes))
        return false;

    secp256k1_pubkey xT1;
    if (!ScalarMultPoint(ctx, x_bytes, T1_point, xT1))
        return false;

    unsigned char x_sq[32];
    ScalarMul(ctx, x_bytes, x_bytes, x_sq);
    if (!secp256k1_ec_seckey_verify(ctx, x_sq))
        return false;

    secp256k1_pubkey x2T2;
    if (!ScalarMultPoint(ctx, x_sq, T2_point, x2T2))
        return false;

    secp256k1_pubkey rhs_partial;
    if (!PointAdd(ctx, z2V, xT1, rhs_partial))
        return false;

    secp256k1_pubkey rhs;
    if (!PointAdd(ctx, rhs_partial, x2T2, rhs))
        return false;

    unsigned char lhs_ser[33], rhs_ser[33];
    size_t lhs_len = 33, rhs_len = 33;
    secp256k1_ec_pubkey_serialize(ctx, lhs_ser, &lhs_len, &lhs, SECP256K1_EC_COMPRESSED);
    secp256k1_ec_pubkey_serialize(ctx, rhs_ser, &rhs_len, &rhs, SECP256K1_EC_COMPRESSED);

    if (memcmp(lhs_ser, rhs_ser, 33) != 0)
        return false;

    uint256 prevChallenge = x_hash;
    for (uint8_t round = 0; round < nRounds; round++)
    {

        unsigned char round_data[33 + 33];
        memcpy(round_data, vpL[round], 33);
        memcpy(round_data + 33, vpR[round], 33);

        uint256 roundChallenge = HashChallengeEx(prevChallenge, round_data, sizeof(round_data));

        if (!secp256k1_ec_seckey_verify(ctx, (const unsigned char*)roundChallenge.begin()))
            return false;

        prevChallenge = roundChallenge;
    }

    bool a_valid = false, b_valid = false;
    for (int i = 0; i < 32; i++)
    {
        if (pA_scalar[i] != 0) a_valid = true;
        if (pB_scalar[i] != 0) b_valid = true;
    }

    if (!b_valid)
        return false;

    memory_cleanse(z_sq, 32);
    memory_cleanse(x_bytes, 32);
    memory_cleanse(x_sq, 32);

    return true;
}

bool BulletproofVerifier::BatchVerify(
    const std::vector<std::pair<Commitment, RangeProof>>& vProofs)
{
    if (vProofs.empty())
        return true;

    for (size_t i = 0; i < vProofs.size(); i++)
    {
        if (!Verify(vProofs[i].first, vProofs[i].second))
            return false;
    }

    return true;
}

std::vector<unsigned char> BulletproofVerifier::ExtractExtraData(
    const RangeProof& proof)
{
    if (proof.IsNull() || proof.GetSize() < 230)
        return std::vector<unsigned char>();

    const unsigned char* pData = proof.data();
    size_t nSize = proof.GetSize();

    uint8_t nBits;
    const unsigned char *pA, *pS, *pT1, *pT2, *pTaux, *pMu;
    uint8_t nRounds;
    std::vector<const unsigned char*> vpL, vpR;
    const unsigned char *pA_scalar, *pB_scalar, *pT_scalar;
    size_t nProofEnd;

    if (!ParseProof(pData, nSize, nBits, pA, pS, pT1, pT2, pTaux, pMu,
                    nRounds, vpL, vpR, pA_scalar, pB_scalar, pT_scalar, nProofEnd))
        return std::vector<unsigned char>();

    if (nProofEnd + 4 > nSize)
        return std::vector<unsigned char>();

    uint32_t extraLen = (uint32_t)pData[nProofEnd]
                      | ((uint32_t)pData[nProofEnd + 1] << 8)
                      | ((uint32_t)pData[nProofEnd + 2] << 16)
                      | ((uint32_t)pData[nProofEnd + 3] << 24);

    size_t extraStart = nProofEnd + 4;

    if (extraLen == 0 || extraStart + extraLen > nSize)
        return std::vector<unsigned char>();

    return std::vector<unsigned char>(pData + extraStart, pData + extraStart + extraLen);
}

bool BulletproofVerifier::RecoverMessage(
    const Commitment& commit,
    const RangeProof& proof,
    const uint256& nonce,
    int64_t& nValueOut,
    BlindingFactor& blindOut,
    std::vector<unsigned char>& vMessageOut)
{
    nValueOut = 0;
    vMessageOut.clear();

    if (proof.IsNull() || proof.GetSize() < 230)
        return false;

    PedersenContext& pedersen = PedersenContext::Get();
    secp256k1_context* ctx = pedersen.GetContext();

    const unsigned char* pData = proof.data();
    size_t nSize = proof.GetSize();

    uint8_t nBits;
    const unsigned char *pA, *pS, *pT1, *pT2, *pTaux, *pMu;
    uint8_t nRounds;
    std::vector<const unsigned char*> vpL, vpR;
    const unsigned char *pA_scalar, *pB_scalar, *pT_scalar;
    size_t nProofEnd;

    if (!ParseProof(pData, nSize, nBits, pA, pS, pT1, pT2, pTaux, pMu,
                    nRounds, vpL, vpR, pA_scalar, pB_scalar, pT_scalar, nProofEnd))
        return false;

    uint64_t uValue;
    if (!ScalarToValue(pA_scalar, uValue))
        return false;

    unsigned char blindData[32];
    {
        SHA256_CTX sha;
        SHA256_Init(&sha);
        SHA256_Update(&sha, nonce.begin(), 32);
        SHA256_Update(&sha, "blind_recover", 13);
        SHA256_Final(blindData, &sha);
    }

    BlindingFactor testBlind(blindData);

    Commitment testCommit = pedersen.Commit((int64_t)uValue, testBlind);

    if (testCommit == commit)
    {
        nValueOut = (int64_t)uValue;
        blindOut = testBlind;
    }
    else
    {

        bool found = false;
        for (uint32_t idx = 0; idx < 256 && !found; idx++)
        {
            SHA256_CTX sha;
            SHA256_Init(&sha);
            SHA256_Update(&sha, nonce.begin(), 32);
            SHA256_Update(&sha, &idx, sizeof(idx));
            SHA256_Update(&sha, "blind", 5);
            SHA256_Final(blindData, &sha);

            if (!secp256k1_ec_seckey_verify(ctx, blindData))
                continue;

            BlindingFactor candidateBlind(blindData);
            Commitment candidateCommit = pedersen.Commit((int64_t)uValue, candidateBlind);

            if (candidateCommit == commit)
            {
                nValueOut = (int64_t)uValue;
                blindOut = candidateBlind;
                found = true;
            }
        }

        if (!found)
        {
            memory_cleanse(blindData, 32);
            return false;
        }
    }

    memory_cleanse(blindData, 32);

    vMessageOut = ExtractExtraData(proof);

    return true;
}

}
}

#endif
