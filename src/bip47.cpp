#include "bip47.h"
#include "stealth.h"
#include "util.h"
#include "base58.h"

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>

#include <cstring>
#include <stdexcept>

static bool BIP47PointToCompressedBytes(const EC_GROUP* pGroup,
                                        const EC_POINT* pPoint,
                                        std::vector<unsigned char>& vchOut)
{
    BN_CTX* pCtx = BN_CTX_new();
    if (!pCtx)
        return false;

    size_t nLen = EC_POINT_point2oct(pGroup, pPoint,
                                     POINT_CONVERSION_COMPRESSED,
                                     NULL, 0, pCtx);
    if (nLen != 33)
    {
        BN_CTX_free(pCtx);
        return false;
    }

    vchOut.resize(33);
    if (EC_POINT_point2oct(pGroup, pPoint,
                           POINT_CONVERSION_COMPRESSED,
                           &vchOut[0], 33, pCtx) != 33)
    {
        BN_CTX_free(pCtx);
        return false;
    }

    BN_CTX_free(pCtx);
    return true;
}

static EC_POINT* BIP47CompressedBytesToPoint(const EC_GROUP* pGroup,
                                             const std::vector<unsigned char>& vchPubKey)
{
    if (vchPubKey.size() != 33)
        return NULL;

    BN_CTX* pCtx = BN_CTX_new();
    if (!pCtx)
        return NULL;

    EC_POINT* pPoint = EC_POINT_new(pGroup);
    if (!pPoint)
    {
        BN_CTX_free(pCtx);
        return NULL;
    }

    if (!EC_POINT_oct2point(pGroup, pPoint, &vchPubKey[0], 33, pCtx))
    {
        EC_POINT_free(pPoint);
        BN_CTX_free(pCtx);
        return NULL;
    }

    BN_CTX_free(pCtx);
    return pPoint;
}

bool CPaymentCode::IsValid() const
{
    return nVersion == BIP47_VERSION &&
           scanPubKey.IsValid() && scanPubKey.IsCompressed() &&
           spendPubKey.IsValid() && spendPubKey.IsCompressed();
}

bool CPaymentCode::Encode(std::vector<unsigned char>& vchPayload) const
{
    if (!IsValid())
        return false;

    vchPayload.resize(PAYMENT_CODE_SIZE, 0);

    vchPayload[0] = nVersion;
    vchPayload[1] = nFeatures;

    const std::vector<unsigned char> vchScan  = scanPubKey.Raw();
    const std::vector<unsigned char> vchSpend = spendPubKey.Raw();

    if (vchScan.size() != 33 || vchSpend.size() != 33)
        return false;

    memcpy(&vchPayload[2],  &vchScan[0],  33);
    memcpy(&vchPayload[35], &vchSpend[0], 33);

    return true;
}

bool CPaymentCode::Decode(const std::vector<unsigned char>& vchPayload)
{
    if (vchPayload.size() != PAYMENT_CODE_SIZE)
        return false;

    nVersion  = vchPayload[0];
    nFeatures = vchPayload[1];

    if (nVersion != BIP47_VERSION)
        return false;

    std::vector<unsigned char> vchScan(vchPayload.begin() + 2,  vchPayload.begin() + 35);
    std::vector<unsigned char> vchSpend(vchPayload.begin() + 35, vchPayload.begin() + 68);

    scanPubKey  = CPubKey(vchScan);
    spendPubKey = CPubKey(vchSpend);

    return IsValid();
}

std::string CPaymentCode::ToBase58() const
{
    std::vector<unsigned char> vchPayload;
    if (!Encode(vchPayload))
        return "";

    std::vector<unsigned char> vchRaw;
    vchRaw.reserve(81);
    vchRaw.push_back(BIP47_BASE58_VERSION);
    vchRaw.insert(vchRaw.end(), vchPayload.begin(), vchPayload.end());

    return EncodeBase58Check(vchRaw);
}

bool CPaymentCode::FromBase58(const std::string& strEncoded)
{
    std::vector<unsigned char> vchRaw;
    if (!DecodeBase58Check(strEncoded, vchRaw))
        return false;

    if (vchRaw.size() != 81)
        return false;

    if (vchRaw[0] != BIP47_BASE58_VERSION)
        return false;

    std::vector<unsigned char> vchPayload(vchRaw.begin() + 1, vchRaw.end());
    return Decode(vchPayload);
}

CKeyID CPaymentCode::GetNotificationAddress() const
{

    if (!spendPubKey.IsValid())
        return CKeyID();
    return spendPubKey.GetID();
}

bool CPaymentCode::DeriveReceiveAddress(const uint256& sharedSecret,
                                        uint32_t nIndex,
                                        CPubKey& destPubKeyOut,
                                        CKeyID& destKeyIDOut) const
{
    if (!IsValid())
        return false;

    uint256 tweak;
    if (!ComputeBIP47AddressTweak(sharedSecret, nIndex, tweak))
        return false;

    return DeriveChildPubKey(spendPubKey, tweak, destPubKeyOut, destKeyIDOut);
}

bool CPaymentCode::DeriveSpendKey(const CKey& spendSecret,
                                  const uint256& sharedSecret,
                                  uint32_t nIndex,
                                  CKey& destKeyOut)
{

    uint256 tweak;
    if (!ComputeBIP47AddressTweak(sharedSecret, nIndex, tweak))
        return false;

    return DeriveChildPrivKey(spendSecret, tweak, destKeyOut);
}

bool ComputeBIP47SharedSecret(const CKey& privkey,
                              const CPubKey& pubkey,
                              uint256& sharedSecretOut)
{

    return ComputeStealthSharedSecret(privkey, pubkey, sharedSecretOut);
}

bool ComputeBIP47AddressTweak(const uint256& sharedSecret,
                              uint32_t nIndex,
                              uint256& tweakOut)
{

    unsigned char vchData[36];
    memcpy(vchData, &sharedSecret, 32);

    vchData[32] = (nIndex >>  0) & 0xFF;
    vchData[33] = (nIndex >>  8) & 0xFF;
    vchData[34] = (nIndex >> 16) & 0xFF;
    vchData[35] = (nIndex >> 24) & 0xFF;

    SHA256(vchData, 36, (unsigned char*)&tweakOut);

    memset(vchData, 0, 36);

    return true;
}

bool DeriveChildPubKey(const CPubKey& spendPubKey,
                       const uint256& tweak,
                       CPubKey& childPubKeyOut,
                       CKeyID& childKeyIDOut)
{
    if (!spendPubKey.IsValid() || !spendPubKey.IsCompressed())
        return false;

    EC_KEY* pEcKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pEcKey)
        return false;

    const EC_GROUP* pGroup = EC_KEY_get0_group(pEcKey);

    EC_POINT* pSpendPoint = BIP47CompressedBytesToPoint(pGroup, spendPubKey.Raw());
    if (!pSpendPoint)
    {
        EC_KEY_free(pEcKey);
        return false;
    }

    BIGNUM* pTweak = BN_bin2bn((const unsigned char*)&tweak, 32, BN_new());
    if (!pTweak)
    {
        EC_POINT_free(pSpendPoint);
        EC_KEY_free(pEcKey);
        return false;
    }

    BN_CTX*   pCtx     = BN_CTX_new();
    EC_POINT* pTweakG  = EC_POINT_new(pGroup);
    EC_POINT* pChild   = EC_POINT_new(pGroup);

    if (!pCtx || !pTweakG || !pChild)
    {
        if (pCtx)    BN_CTX_free(pCtx);
        if (pTweakG) EC_POINT_free(pTweakG);
        if (pChild)  EC_POINT_free(pChild);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pTweak);
        EC_KEY_free(pEcKey);
        return false;
    }

    if (!EC_POINT_mul(pGroup, pTweakG, pTweak, NULL, NULL, pCtx))
    {
        BN_CTX_free(pCtx);
        EC_POINT_free(pTweakG);
        EC_POINT_free(pChild);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pTweak);
        EC_KEY_free(pEcKey);
        return false;
    }

    if (!EC_POINT_add(pGroup, pChild, pSpendPoint, pTweakG, pCtx))
    {
        BN_CTX_free(pCtx);
        EC_POINT_free(pTweakG);
        EC_POINT_free(pChild);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pTweak);
        EC_KEY_free(pEcKey);
        return false;
    }

    std::vector<unsigned char> vchChild;
    if (!BIP47PointToCompressedBytes(pGroup, pChild, vchChild))
    {
        BN_CTX_free(pCtx);
        EC_POINT_free(pTweakG);
        EC_POINT_free(pChild);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pTweak);
        EC_KEY_free(pEcKey);
        return false;
    }

    childPubKeyOut = CPubKey(vchChild);
    childKeyIDOut  = childPubKeyOut.GetID();

    BN_CTX_free(pCtx);
    EC_POINT_free(pTweakG);
    EC_POINT_free(pChild);
    EC_POINT_free(pSpendPoint);
    BN_clear_free(pTweak);
    EC_KEY_free(pEcKey);
    return true;
}

bool DeriveChildPrivKey(const CKey& spendSecret,
                        const uint256& tweak,
                        CKey& childKeyOut)
{

    bool fCompressed = false;
    CSecret vchSpendSecret = spendSecret.GetSecret(fCompressed);
    if (vchSpendSecret.size() != 32)
        return false;

    EC_KEY* pEcKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pEcKey)
        return false;

    const EC_GROUP* pGroup = EC_KEY_get0_group(pEcKey);
    BN_CTX* pCtx    = BN_CTX_new();
    BIGNUM* pOrder  = BN_new();
    BIGNUM* pSpend  = BN_bin2bn(&vchSpendSecret[0], 32, BN_new());
    BIGNUM* pTweak  = BN_bin2bn((const unsigned char*)&tweak, 32, BN_new());
    BIGNUM* pResult = BN_new();

    if (!pCtx || !pOrder || !pSpend || !pTweak || !pResult ||
        !EC_GROUP_get_order(pGroup, pOrder, pCtx))
    {
        if (pCtx)    BN_CTX_free(pCtx);
        if (pOrder)  BN_free(pOrder);
        if (pSpend)  BN_clear_free(pSpend);
        if (pTweak)  BN_clear_free(pTweak);
        if (pResult) BN_clear_free(pResult);
        EC_KEY_free(pEcKey);
        return false;
    }

    if (!BN_mod_add(pResult, pSpend, pTweak, pOrder, pCtx))
    {
        BN_CTX_free(pCtx);
        BN_free(pOrder);
        BN_clear_free(pSpend);
        BN_clear_free(pTweak);
        BN_clear_free(pResult);
        EC_KEY_free(pEcKey);
        return false;
    }

    unsigned char vchResult[32];
    memset(vchResult, 0, 32);
    int nBytes = BN_num_bytes(pResult);
    if (nBytes > 32)
    {
        BN_CTX_free(pCtx);
        BN_free(pOrder);
        BN_clear_free(pSpend);
        BN_clear_free(pTweak);
        BN_clear_free(pResult);
        EC_KEY_free(pEcKey);
        return false;
    }
    BN_bn2bin(pResult, vchResult + (32 - nBytes));

    CSecret vchChildSecret(vchResult, vchResult + 32);
    memset(vchResult, 0, 32);

    bool fOk = childKeyOut.SetSecret(vchChildSecret, true );

    BN_CTX_free(pCtx);
    BN_free(pOrder);
    BN_clear_free(pSpend);
    BN_clear_free(pTweak);
    BN_clear_free(pResult);
    EC_KEY_free(pEcKey);
    return fOk;
}

static bool GenerateBlindingMask(const uint256& blindingSecret,
                                 std::vector<unsigned char>& vchMask)
{
    vchMask.resize(PAYMENT_CODE_SIZE);

    unsigned char vchSha512[64];
    SHA512((const unsigned char*)&blindingSecret, 32, vchSha512);
    memcpy(&vchMask[0], vchSha512, 64);

    unsigned char vchExtra[32];
    SHA256(vchSha512, 64, vchExtra);
    memcpy(&vchMask[64], vchExtra, 16);

    memset(vchSha512, 0, 64);
    memset(vchExtra, 0, 32);

    return true;
}

bool BlindPaymentCode(const std::vector<unsigned char>& vchPayload,
                      const uint256& blindingSecret,
                      std::vector<unsigned char>& vchBlindedOut)
{
    if (vchPayload.size() != PAYMENT_CODE_SIZE)
        return false;

    std::vector<unsigned char> vchMask;
    if (!GenerateBlindingMask(blindingSecret, vchMask))
        return false;

    vchBlindedOut.resize(PAYMENT_CODE_SIZE);
    for (size_t i = 0; i < PAYMENT_CODE_SIZE; ++i)
        vchBlindedOut[i] = vchPayload[i] ^ vchMask[i];

    return true;
}

bool UnblindPaymentCode(const std::vector<unsigned char>& vchBlinded,
                        const uint256& blindingSecret,
                        std::vector<unsigned char>& vchPayloadOut)
{

    return BlindPaymentCode(vchBlinded, blindingSecret, vchPayloadOut);
}

CScript BuildNotificationScript(const std::vector<unsigned char>& vchBlindedPayload)
{
    CScript script;
    if (vchBlindedPayload.size() == PAYMENT_CODE_SIZE)
    {
        script << OP_RETURN << vchBlindedPayload;
    }
    return script;
}

bool ExtractNotificationPayload(const CScript& script,
                                std::vector<unsigned char>& vchPayloadOut)
{

    CScript::const_iterator pc = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vchData;

    if (!script.GetOp(pc, opcode, vchData))
        return false;
    if (opcode != OP_RETURN)
        return false;

    if (!script.GetOp(pc, opcode, vchData))
        return false;

    if (vchData.size() != PAYMENT_CODE_SIZE)
        return false;

    if (pc != script.end())
        return false;

    vchPayloadOut = vchData;
    return true;
}

bool ComputeBlindingSecret(const CKey& senderScanPriv,
                           const CPubKey& receiverScanPub,
                           uint256& blindingSecretOut)
{

    return ComputeBIP47SharedSecret(senderScanPriv, receiverScanPub, blindingSecretOut);
}
