#include "stealth.h"
#include "util.h"
#include "base58.h"

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>

#include <cstring>
#include <stdexcept>

static bool PointToCompressedBytes(const EC_GROUP* pGroup,
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

static EC_POINT* CompressedBytesToPoint(const EC_GROUP* pGroup,
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

bool CStealthAddress::IsValid() const
{
    return scanPubKey.IsValid() && scanPubKey.IsCompressed() &&
           spendPubKey.IsValid() && spendPubKey.IsCompressed();
}

std::string CStealthAddress::Encoded() const
{
    if (!IsValid())
        return "";

    std::vector<unsigned char> vchRaw;
    vchRaw.reserve(67);
    vchRaw.push_back(STEALTH_VERSION);

    const std::vector<unsigned char> vchScan  = scanPubKey.Raw();
    const std::vector<unsigned char> vchSpend = spendPubKey.Raw();

    vchRaw.insert(vchRaw.end(), vchScan.begin(),  vchScan.end());
    vchRaw.insert(vchRaw.end(), vchSpend.begin(), vchSpend.end());

    return EncodeBase58Check(vchRaw);
}

bool CStealthAddress::SetEncoded(const std::string& addr)
{
    std::vector<unsigned char> vchRaw;
    if (!DecodeBase58Check(addr, vchRaw))
        return false;

    if (vchRaw.size() != 67)
        return false;

    if (vchRaw[0] != STEALTH_VERSION)
        return false;

    std::vector<unsigned char> vchScan(vchRaw.begin() + 1,  vchRaw.begin() + 34);
    std::vector<unsigned char> vchSpend(vchRaw.begin() + 34, vchRaw.begin() + 67);

    scanPubKey  = CPubKey(vchScan);
    spendPubKey = CPubKey(vchSpend);

    return IsValid();
}

bool ComputeStealthSharedSecret(const CKey& privkey,
                                const CPubKey& pubkey,
                                uint256& sharedSecretOut)
{
    if (!pubkey.IsValid() || !pubkey.IsCompressed())
        return false;

    EC_KEY* pEcKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pEcKey)
        return false;

    const EC_GROUP* pGroup = EC_KEY_get0_group(pEcKey);

    bool fCompressed = false;
    CSecret vchSecret = privkey.GetSecret(fCompressed);
    if (vchSecret.size() != 32)
    {
        EC_KEY_free(pEcKey);
        return false;
    }

    BIGNUM* pPrivBN = BN_bin2bn(&vchSecret[0], 32, BN_new());
    if (!pPrivBN)
    {
        EC_KEY_free(pEcKey);
        return false;
    }

    EC_POINT* pPeerPoint = CompressedBytesToPoint(pGroup, pubkey.Raw());
    if (!pPeerPoint)
    {
        BN_clear_free(pPrivBN);
        EC_KEY_free(pEcKey);
        return false;
    }

    BN_CTX* pCtx = BN_CTX_new();
    EC_POINT* pSharedPoint = EC_POINT_new(pGroup);
    if (!pCtx || !pSharedPoint)
    {
        if (pCtx) BN_CTX_free(pCtx);
        if (pSharedPoint) EC_POINT_free(pSharedPoint);
        EC_POINT_free(pPeerPoint);
        BN_clear_free(pPrivBN);
        EC_KEY_free(pEcKey);
        return false;
    }

    if (!EC_POINT_mul(pGroup, pSharedPoint, NULL, pPeerPoint, pPrivBN, pCtx))
    {
        BN_CTX_free(pCtx);
        EC_POINT_free(pSharedPoint);
        EC_POINT_free(pPeerPoint);
        BN_clear_free(pPrivBN);
        EC_KEY_free(pEcKey);
        return false;
    }

    BIGNUM* pX = BN_new();
    BIGNUM* pY = BN_new();
    if (!pX || !pY ||
        !EC_POINT_get_affine_coordinates(pGroup, pSharedPoint, pX, pY, pCtx))
    {
        if (pX) BN_free(pX);
        if (pY) BN_free(pY);
        BN_CTX_free(pCtx);
        EC_POINT_free(pSharedPoint);
        EC_POINT_free(pPeerPoint);
        BN_clear_free(pPrivBN);
        EC_KEY_free(pEcKey);
        return false;
    }

    unsigned char vchX[32];
    memset(vchX, 0, 32);
    int nBytes = BN_num_bytes(pX);
    if (nBytes > 32)
    {
        BN_free(pX);
        BN_free(pY);
        BN_CTX_free(pCtx);
        EC_POINT_free(pSharedPoint);
        EC_POINT_free(pPeerPoint);
        BN_clear_free(pPrivBN);
        EC_KEY_free(pEcKey);
        return false;
    }
    BN_bn2bin(pX, vchX + (32 - nBytes));

    SHA256(vchX, 32, (unsigned char*)&sharedSecretOut);

    memset(vchX, 0, 32);

    BN_free(pX);
    BN_free(pY);
    BN_CTX_free(pCtx);
    EC_POINT_free(pSharedPoint);
    EC_POINT_free(pPeerPoint);
    BN_clear_free(pPrivBN);
    EC_KEY_free(pEcKey);
    return true;
}

bool DeriveStealthDestination(const CPubKey& spendPubKey,
                               const uint256& sharedSecret,
                               CPubKey& destPubKeyOut,
                               CKeyID& destKeyIDOut)
{
    if (!spendPubKey.IsValid() || !spendPubKey.IsCompressed())
        return false;

    EC_KEY* pEcKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pEcKey)
        return false;

    const EC_GROUP* pGroup = EC_KEY_get0_group(pEcKey);

    EC_POINT* pSpendPoint = CompressedBytesToPoint(pGroup, spendPubKey.Raw());
    if (!pSpendPoint)
    {
        EC_KEY_free(pEcKey);
        return false;
    }

    unsigned char vchHash[32];
    SHA256((const unsigned char*)&sharedSecret, 32, vchHash);

    BIGNUM* pScalar = BN_bin2bn(vchHash, 32, BN_new());
    memset(vchHash, 0, 32);

    if (!pScalar)
    {
        EC_POINT_free(pSpendPoint);
        EC_KEY_free(pEcKey);
        return false;
    }

    BN_CTX* pCtx   = BN_CTX_new();
    EC_POINT* pScalarG = EC_POINT_new(pGroup);
    EC_POINT* pDest    = EC_POINT_new(pGroup);

    if (!pCtx || !pScalarG || !pDest)
    {
        if (pCtx)     BN_CTX_free(pCtx);
        if (pScalarG) EC_POINT_free(pScalarG);
        if (pDest)    EC_POINT_free(pDest);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pScalar);
        EC_KEY_free(pEcKey);
        return false;
    }

    if (!EC_POINT_mul(pGroup, pScalarG, pScalar, NULL, NULL, pCtx))
    {
        BN_CTX_free(pCtx);
        EC_POINT_free(pScalarG);
        EC_POINT_free(pDest);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pScalar);
        EC_KEY_free(pEcKey);
        return false;
    }

    if (!EC_POINT_add(pGroup, pDest, pSpendPoint, pScalarG, pCtx))
    {
        BN_CTX_free(pCtx);
        EC_POINT_free(pScalarG);
        EC_POINT_free(pDest);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pScalar);
        EC_KEY_free(pEcKey);
        return false;
    }

    std::vector<unsigned char> vchDest;
    if (!PointToCompressedBytes(pGroup, pDest, vchDest))
    {
        BN_CTX_free(pCtx);
        EC_POINT_free(pScalarG);
        EC_POINT_free(pDest);
        EC_POINT_free(pSpendPoint);
        BN_clear_free(pScalar);
        EC_KEY_free(pEcKey);
        return false;
    }

    destPubKeyOut = CPubKey(vchDest);
    destKeyIDOut  = destPubKeyOut.GetID();

    BN_CTX_free(pCtx);
    EC_POINT_free(pScalarG);
    EC_POINT_free(pDest);
    EC_POINT_free(pSpendPoint);
    BN_clear_free(pScalar);
    EC_KEY_free(pEcKey);
    return true;
}

bool DeriveStealthSpendKey(const CKey& spendSecret,
                           const uint256& sharedSecret,
                           CKey& destKeyOut)
{

    bool fCompressed = false;
    CSecret vchSpendSecret = spendSecret.GetSecret(fCompressed);
    if (vchSpendSecret.size() != 32)
        return false;

    unsigned char vchHash[32];
    SHA256((const unsigned char*)&sharedSecret, 32, vchHash);

    EC_KEY* pEcKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pEcKey)
    {
        memset(vchHash, 0, 32);
        return false;
    }

    const EC_GROUP* pGroup = EC_KEY_get0_group(pEcKey);
    BN_CTX* pCtx   = BN_CTX_new();
    BIGNUM* pOrder  = BN_new();
    BIGNUM* pSpend  = BN_bin2bn(&vchSpendSecret[0], 32, BN_new());
    BIGNUM* pAdd    = BN_bin2bn(vchHash, 32, BN_new());
    BIGNUM* pResult = BN_new();

    memset(vchHash, 0, 32);

    if (!pCtx || !pOrder || !pSpend || !pAdd || !pResult ||
        !EC_GROUP_get_order(pGroup, pOrder, pCtx))
    {
        if (pCtx)    BN_CTX_free(pCtx);
        if (pOrder)  BN_free(pOrder);
        if (pSpend)  BN_clear_free(pSpend);
        if (pAdd)    BN_clear_free(pAdd);
        if (pResult) BN_clear_free(pResult);
        EC_KEY_free(pEcKey);
        return false;
    }

    if (!BN_mod_add(pResult, pSpend, pAdd, pOrder, pCtx))
    {
        BN_CTX_free(pCtx);
        BN_free(pOrder);
        BN_clear_free(pSpend);
        BN_clear_free(pAdd);
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
        BN_clear_free(pAdd);
        BN_clear_free(pResult);
        EC_KEY_free(pEcKey);
        return false;
    }
    BN_bn2bin(pResult, vchResult + (32 - nBytes));

    CSecret vchDestSecret(vchResult, vchResult + 32);
    memset(vchResult, 0, 32);

    bool fOk = destKeyOut.SetSecret(vchDestSecret, true );

    BN_CTX_free(pCtx);
    BN_free(pOrder);
    BN_clear_free(pSpend);
    BN_clear_free(pAdd);
    BN_clear_free(pResult);
    EC_KEY_free(pEcKey);
    return fOk;
}

bool GenerateStealthAddress(CStealthAddress& addressOut,
                            CKey& scanSecretOut,
                            CKey& spendSecretOut)
{

    scanSecretOut.MakeNewKey(true );
    spendSecretOut.MakeNewKey(true );

    if (!scanSecretOut.IsValid() || !spendSecretOut.IsValid())
        return false;

    addressOut = CStealthAddress(scanSecretOut.GetPubKey(),
                                 spendSecretOut.GetPubKey());

    return addressOut.IsValid();
}

bool PrepareStealthPayment(const CStealthAddress& address,
                           CStealthPayment& paymentOut)
{
    if (!address.IsValid())
        return false;

    CKey ephemeralKey;
    ephemeralKey.MakeNewKey(true );
    if (!ephemeralKey.IsValid())
        return false;

    paymentOut.ephemeralPubKey = ephemeralKey.GetPubKey();

    uint256 sharedSecret;
    if (!ComputeStealthSharedSecret(ephemeralKey, address.scanPubKey, sharedSecret))
        return false;

    if (!DeriveStealthDestination(address.spendPubKey, sharedSecret,
                                  paymentOut.destPubKey,
                                  paymentOut.destKeyID))
        return false;

    return true;
}

bool DetectStealthPayment(const CKey& scanSecret,
                          const CPubKey& ephemeralPubKey,
                          const CPubKey& spendPubKey,
                          CKey& destKeyOut)
{
    if (!ephemeralPubKey.IsValid() || !spendPubKey.IsValid())
        return false;

    uint256 sharedSecret;
    if (!ComputeStealthSharedSecret(scanSecret, ephemeralPubKey, sharedSecret))
        return false;

    CPubKey destPubKey;
    CKeyID  destKeyID;
    if (!DeriveStealthDestination(spendPubKey, sharedSecret, destPubKey, destKeyID))
        return false;

    if (!destKeyOut.SetPubKey(destPubKey))
        return false;

    return true;
}
