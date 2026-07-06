#ifdef ENABLE_MWEB

#include "mw/mw_wallet.h"
#include "mw/crypto/pedersen.h"
#include "mw/crypto/schnorr.h"
#include "mw/crypto/bulletproofs.h"
#include "wallet.h"
#include "walletdb.h"
#include "util.h"
#include "hash.h"

#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/ecdh.h>

using namespace std;

namespace mw {

static bool ComputeECDHSharedSecret(const SecretKey& privKey,
                                     const unsigned char* pPubKey33,
                                     uint256& sharedSecretOut)
{

    EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!eckey)
        return false;

    const EC_GROUP* group = EC_KEY_get0_group(eckey);
    BIGNUM* bn_priv = BN_bin2bn(privKey.data, 32, NULL);
    if (!bn_priv)
    {
        EC_KEY_free(eckey);
        return false;
    }
    EC_KEY_set_private_key(eckey, bn_priv);

    EC_POINT* peerPoint = EC_POINT_new(group);
    if (!peerPoint)
    {
        BN_free(bn_priv);
        EC_KEY_free(eckey);
        return false;
    }

    if (!EC_POINT_oct2point(group, peerPoint, pPubKey33, PUBKEY_SIZE, NULL))
    {
        EC_POINT_free(peerPoint);
        BN_free(bn_priv);
        EC_KEY_free(eckey);
        return false;
    }

    EC_POINT* sharedPoint = EC_POINT_new(group);
    if (!sharedPoint)
    {
        EC_POINT_free(peerPoint);
        BN_free(bn_priv);
        EC_KEY_free(eckey);
        return false;
    }

    BN_CTX* ctx = BN_CTX_new();
    if (!EC_POINT_mul(group, sharedPoint, NULL, peerPoint, bn_priv, ctx))
    {
        BN_CTX_free(ctx);
        EC_POINT_free(sharedPoint);
        EC_POINT_free(peerPoint);
        BN_free(bn_priv);
        EC_KEY_free(eckey);
        return false;
    }

    unsigned char sharedPointBytes[PUBKEY_SIZE];
    size_t len = EC_POINT_point2oct(group, sharedPoint, POINT_CONVERSION_COMPRESSED,
                                     sharedPointBytes, PUBKEY_SIZE, ctx);

    BN_CTX_free(ctx);
    EC_POINT_free(sharedPoint);
    EC_POINT_free(peerPoint);
    BN_free(bn_priv);
    EC_KEY_free(eckey);

    if (len != PUBKEY_SIZE)
        return false;

    SHA256(sharedPointBytes, PUBKEY_SIZE, (unsigned char*)&sharedSecretOut);
    return true;
}

static bool DerivePublicKey(const SecretKey& privKey, unsigned char* pPubKey33)
{
    EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!eckey)
        return false;

    const EC_GROUP* group = EC_KEY_get0_group(eckey);
    BIGNUM* bn_priv = BN_bin2bn(privKey.data, 32, NULL);
    if (!bn_priv)
    {
        EC_KEY_free(eckey);
        return false;
    }

    EC_KEY_set_private_key(eckey, bn_priv);

    EC_POINT* pubPoint = EC_POINT_new(group);
    BN_CTX* ctx = BN_CTX_new();
    EC_POINT_mul(group, pubPoint, bn_priv, NULL, NULL, ctx);
    EC_KEY_set_public_key(eckey, pubPoint);

    size_t len = EC_POINT_point2oct(group, pubPoint, POINT_CONVERSION_COMPRESSED,
                                     pPubKey33, PUBKEY_SIZE, ctx);

    BN_CTX_free(ctx);
    EC_POINT_free(pubPoint);
    BN_free(bn_priv);
    EC_KEY_free(eckey);

    return (len == PUBKEY_SIZE);
}

static bool AddScalarToPublicKey(const unsigned char* pPubKey33,
                                  const uint256& scalar,
                                  unsigned char* pResultPubKey33)
{
    EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!eckey)
        return false;

    const EC_GROUP* group = EC_KEY_get0_group(eckey);
    BN_CTX* ctx = BN_CTX_new();

    EC_POINT* basePoint = EC_POINT_new(group);
    if (!EC_POINT_oct2point(group, basePoint, pPubKey33, PUBKEY_SIZE, ctx))
    {
        EC_POINT_free(basePoint);
        BN_CTX_free(ctx);
        EC_KEY_free(eckey);
        return false;
    }

    BIGNUM* bn_scalar = BN_bin2bn((const unsigned char*)scalar.begin(), 32, NULL);
    EC_POINT* scalarG = EC_POINT_new(group);
    EC_POINT_mul(group, scalarG, bn_scalar, NULL, NULL, ctx);

    EC_POINT* resultPoint = EC_POINT_new(group);
    EC_POINT_add(group, resultPoint, basePoint, scalarG, ctx);

    size_t len = EC_POINT_point2oct(group, resultPoint, POINT_CONVERSION_COMPRESSED,
                                     pResultPubKey33, PUBKEY_SIZE, ctx);

    EC_POINT_free(resultPoint);
    EC_POINT_free(scalarG);
    BN_free(bn_scalar);
    EC_POINT_free(basePoint);
    BN_CTX_free(ctx);
    EC_KEY_free(eckey);

    return (len == PUBKEY_SIZE);
}

static bool AddScalars(const unsigned char* pA, const uint256& b, SecretKey& result)
{
    EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!eckey)
        return false;

    const EC_GROUP* group = EC_KEY_get0_group(eckey);
    BN_CTX* ctx = BN_CTX_new();

    BIGNUM* bn_a = BN_bin2bn(pA, 32, NULL);
    BIGNUM* bn_b = BN_bin2bn((const unsigned char*)b.begin(), 32, NULL);
    BIGNUM* bn_result = BN_new();

    BIGNUM* order = BN_new();
    EC_GROUP_get_order(group, order, ctx);

    BN_mod_add(bn_result, bn_a, bn_b, order, ctx);

    memset(result.data, 0, 32);
    int nBytes = BN_num_bytes(bn_result);
    if (nBytes > 32)
    {
        BN_free(bn_result);
        BN_free(bn_b);
        BN_free(bn_a);
        BN_CTX_free(ctx);
        EC_KEY_free(eckey);
        return false;
    }
    BN_bn2bin(bn_result, result.data + (32 - nBytes));

    BN_free(bn_result);
    BN_free(bn_b);
    BN_free(bn_a);
    BN_CTX_free(ctx);
    EC_KEY_free(eckey);

    return true;
}

void CMWWallet::Init(CWallet* pWalletIn)
{
    LOCK(cs_mwwallet);
    pWallet = pWalletIn;
}

bool CMWWallet::GenerateKeys()
{
    LOCK(cs_mwwallet);

    if (HasKeys())
        return true;

    if (RAND_bytes(scanPrivKey.data, 32) != 1)
        return false;

    if (RAND_bytes(spendPrivKey.data, 32) != 1)
        return false;

    if (!DerivePublicKey(scanPrivKey, scanPubKey))
        return false;

    if (!DerivePublicKey(spendPrivKey, spendPubKey))
        return false;

    printf("CMWWallet::GenerateKeys() : MW scan/spend key pair generated\n");

    return SaveToWalletDB();
}

bool CMWWallet::HasKeys() const
{
    return !scanPrivKey.IsNull() && !spendPrivKey.IsNull();
}

bool CMWWallet::DeriveSpendKey(const unsigned char* pSenderPubKey,
                                SecretKey& keyOut) const
{

    uint256 sharedSecret;
    if (!ComputeECDHSharedSecret(scanPrivKey, pSenderPubKey, sharedSecret))
        return false;

    if (!AddScalars(spendPrivKey.data, sharedSecret, keyOut))
        return false;

    return true;
}

int64_t CMWWallet::GetMWBalance() const
{
    LOCK(cs_mwwallet);

    int64_t nBalance = 0;
    for (map<Commitment, CMWOwnedOutput>::const_iterator it = mapOwnedOutputs.begin();
         it != mapOwnedOutputs.end(); ++it)
    {
        if (!it->second.fSpent)
            nBalance += it->second.nValue;
    }
    return nBalance;
}

int64_t CMWWallet::GetMWConfirmedBalance(int nMinDepth) const
{
    LOCK(cs_mwwallet);

    int nCurrentHeight = nBestHeight;
    int64_t nBalance = 0;
    for (map<Commitment, CMWOwnedOutput>::const_iterator it = mapOwnedOutputs.begin();
         it != mapOwnedOutputs.end(); ++it)
    {
        if (!it->second.fSpent && it->second.nBlockHeight > 0)
        {
            int nDepth = nCurrentHeight - it->second.nBlockHeight + 1;
            if (nDepth >= nMinDepth)
                nBalance += it->second.nValue;
        }
    }
    return nBalance;
}

int64_t CMWWallet::GetMWUnconfirmedBalance() const
{
    LOCK(cs_mwwallet);

    int64_t nBalance = 0;
    for (map<Commitment, CMWOwnedOutput>::const_iterator it = mapOwnedOutputs.begin();
         it != mapOwnedOutputs.end(); ++it)
    {
        if (!it->second.fSpent && it->second.nBlockHeight <= 0)
            nBalance += it->second.nValue;
    }
    return nBalance;
}

bool CMWWallet::CreateMWTransaction(const unsigned char* pDestScanKey,
                                     const unsigned char* pDestSpendKey,
                                     int64_t nAmount,
                                     CMWTransaction& txOut)
{
    LOCK(cs_mwwallet);

    if (nAmount <= 0)
        return false;

    int64_t nFee = MIN_TX_FEE;

    vector<CMWOwnedOutput> vSelected;
    int64_t nValueSelected = 0;
    if (!SelectMWOutputs(nAmount + nFee, vSelected, nValueSelected))
    {
        printf("CMWWallet::CreateMWTransaction() : insufficient MWEB funds\n");
        return false;
    }

    int64_t nChange = nValueSelected - nAmount - nFee;

    vector<CMWInput> vInputs;
    vector<BlindingFactor> vInputBlinds;

    for (size_t i = 0; i < vSelected.size(); i++)
    {
        CMWInput input;
        input.commitment = vSelected[i].output.commitment;
        memcpy(input.outputPubKey, vSelected[i].output.receiverPubKey, PUBKEY_SIZE);

        vInputs.push_back(input);
        vInputBlinds.push_back(vSelected[i].blindingFactor);
    }

    crypto::PedersenContext& pedersen = crypto::PedersenContext::Get();

    BlindingFactor destBlind = pedersen.GenerateBlindingFactor();

    BlindingFactor changeBlind;
    if (nChange > 0)
        changeBlind = pedersen.GenerateBlindingFactor();

    vector<CMWOutput> vOutputs;

    SecretKey ephemeralSecret;
    RAND_bytes(ephemeralSecret.data, 32);
    unsigned char ephemeralPubKey[PUBKEY_SIZE];
    DerivePublicKey(ephemeralSecret, ephemeralPubKey);

    uint256 destSharedSecret;
    ComputeECDHSharedSecret(ephemeralSecret, pDestScanKey, destSharedSecret);

    unsigned char destOneTimePubKey[PUBKEY_SIZE];
    AddScalarToPublicKey(pDestSpendKey, destSharedSecret, destOneTimePubKey);

    Commitment destCommitment = pedersen.Commit(nAmount, destBlind);

    uint256 proofNonce;
    RAND_bytes((unsigned char*)&proofNonce, 32);
    RangeProof destProof = crypto::BulletproofProver::Prove(nAmount, destBlind, proofNonce);

    CMWOutput destOutput(destCommitment, ephemeralPubKey, destOneTimePubKey,
                         destProof, OUTPUT_STANDARD);
    vOutputs.push_back(destOutput);

    if (nChange > 0)
    {

        SecretKey changeEphemeralSecret;
        RAND_bytes(changeEphemeralSecret.data, 32);
        unsigned char changeEphemeralPub[PUBKEY_SIZE];
        DerivePublicKey(changeEphemeralSecret, changeEphemeralPub);

        uint256 changeSharedSecret;
        ComputeECDHSharedSecret(changeEphemeralSecret, scanPubKey, changeSharedSecret);

        unsigned char changeOneTimePub[PUBKEY_SIZE];
        AddScalarToPublicKey(spendPubKey, changeSharedSecret, changeOneTimePub);

        Commitment changeCommitment = pedersen.Commit(nChange, changeBlind);

        uint256 changeProofNonce;
        RAND_bytes((unsigned char*)&changeProofNonce, 32);
        RangeProof changeProof = crypto::BulletproofProver::Prove(nChange, changeBlind, changeProofNonce);

        CMWOutput changeOutput(changeCommitment, changeEphemeralPub, changeOneTimePub,
                               changeProof, OUTPUT_STANDARD);
        vOutputs.push_back(changeOutput);
    }

    vector<BlindingFactor> vPositiveBlinds;
    vPositiveBlinds.push_back(destBlind);
    if (nChange > 0)
        vPositiveBlinds.push_back(changeBlind);

    BlindingFactor excessBlind = pedersen.BlindSum(vPositiveBlinds, vInputBlinds);

    BlindingFactor offset = pedersen.GenerateBlindingFactor();

    vector<BlindingFactor> vOffsetNeg;
    vOffsetNeg.push_back(offset);
    vector<BlindingFactor> vExcessPos;
    vExcessPos.push_back(excessBlind);
    BlindingFactor adjustedExcess = pedersen.BlindSum(vExcessPos, vOffsetNeg);

    Commitment excessCommitment = pedersen.CommitBlind(adjustedExcess);

    CMWKernel kernel;
    kernel.nFeatures = KERNEL_PLAIN;
    kernel.nFee = nFee;
    kernel.excess = excessCommitment;

    uint256 sigMsg = kernel.GetSignatureMessage();
    kernel.signature = crypto::SchnorrSigner::SignExcess(adjustedExcess, sigMsg);

    vector<CMWKernel> vKernels;
    vKernels.push_back(kernel);

    CMWTransactionBody body(vInputs, vOutputs, vKernels);
    body.Sort();

    txOut = CMWTransaction(body, offset);

    uint256 txHash = txOut.GetHash();
    for (size_t i = 0; i < vSelected.size(); i++)
    {
        Signature inputSig = crypto::SchnorrSigner::Sign(vSelected[i].spendKey, txHash);

        for (size_t j = 0; j < txOut.body.vInputs.size(); j++)
        {
            if (txOut.body.vInputs[j].commitment == vSelected[i].output.commitment)
            {
                txOut.body.vInputs[j].signature = inputSig;
                break;
            }
        }
    }

    for (size_t i = 0; i < vSelected.size(); i++)
    {
        MarkOutputSpent(vSelected[i].output.commitment);
    }

    if (nChange > 0)
    {
        CMWOwnedOutput changeOwned;
        changeOwned.output = vOutputs.back();
        changeOwned.blindingFactor = changeBlind;

        DeriveSpendKey(vOutputs.back().senderPubKey, changeOwned.spendKey);
        changeOwned.nValue = nChange;
        changeOwned.nBlockHeight = 0;
        changeOwned.fSpent = false;
        AddOwnedOutput(changeOwned);
    }

    printf("CMWWallet::CreateMWTransaction() : created MW tx, amount=%" PRId64 ", fee=%" PRId64 ", change=%" PRId64 "\n",
           nAmount, nFee, nChange);

    return true;
}

bool CMWWallet::SelectMWOutputs(int64_t nTargetValue,
                                 vector<CMWOwnedOutput>& vSelected,
                                 int64_t& nValueOut) const
{
    LOCK(cs_mwwallet);

    vSelected.clear();
    nValueOut = 0;

    vector<CMWOwnedOutput> vUnspent;
    for (map<Commitment, CMWOwnedOutput>::const_iterator it = mapOwnedOutputs.begin();
         it != mapOwnedOutputs.end(); ++it)
    {
        if (!it->second.fSpent)
            vUnspent.push_back(it->second);
    }

    sort(vUnspent.begin(), vUnspent.end(),
         [](const CMWOwnedOutput& a, const CMWOwnedOutput& b) {
             return a.nValue > b.nValue;
         });

    for (size_t i = 0; i < vUnspent.size(); i++)
    {
        vSelected.push_back(vUnspent[i]);
        nValueOut += vUnspent[i].nValue;
        if (nValueOut >= nTargetValue)
            return true;
    }

    vSelected.clear();
    nValueOut = 0;
    return false;
}

int CMWWallet::ScanForOutputs(const CMWBlock& block)
{
    LOCK(cs_mwwallet);

    if (!HasKeys())
        return 0;

    int nFound = 0;

    for (size_t i = 0; i < block.body.vOutputs.size(); i++)
    {
        const CMWOutput& output = block.body.vOutputs[i];

        if (mapOwnedOutputs.count(output.commitment) > 0)
            continue;

        uint256 sharedSecret;
        if (!ComputeECDHSharedSecret(scanPrivKey, output.senderPubKey, sharedSecret))
            continue;

        unsigned char expectedReceiverKey[PUBKEY_SIZE];
        if (!AddScalarToPublicKey(spendPubKey, sharedSecret, expectedReceiverKey))
            continue;

        if (memcmp(expectedReceiverKey, output.receiverPubKey, PUBKEY_SIZE) != 0)
            continue;

        BlindingFactor blind(sharedSecret);

        SecretKey oneTimeSpendKey;
        if (!DeriveSpendKey(output.senderPubKey, oneTimeSpendKey))
            continue;

        int64_t nRecoveredValue = 0;

        uint256 proofNonce;
        {
            SHA256_CTX ctx;
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, (unsigned char*)&sharedSecret, 32);
            const char* tag = "nonce";
            SHA256_Update(&ctx, tag, 5);
            SHA256_Final((unsigned char*)&proofNonce, &ctx);
        }

        BlindingFactor recoveredBlind;
        vector<unsigned char> vMessage;
        if (!crypto::BulletproofVerifier::RecoverMessage(
                output.commitment, output.rangeProof, proofNonce,
                nRecoveredValue, recoveredBlind, vMessage))
        {

            if (!crypto::BulletproofVerifier::RecoverMessage(
                    output.commitment, output.rangeProof, sharedSecret,
                    nRecoveredValue, recoveredBlind, vMessage))
            {
                printf("CMWWallet::ScanForOutputs() : matched output but failed to rewind proof\n");
                continue;
            }
        }

        CMWOwnedOutput owned;
        owned.output = output;
        owned.blindingFactor = recoveredBlind;
        owned.spendKey = oneTimeSpendKey;
        owned.nValue = nRecoveredValue;
        owned.nBlockHeight = block.nHeight;
        owned.fSpent = false;

        mapOwnedOutputs[output.commitment] = owned;
        nFound++;

        printf("CMWWallet::ScanForOutputs() : found MW output, value=%" PRId64 " at height %d\n",
               nRecoveredValue, block.nHeight);
    }

    for (size_t i = 0; i < block.body.vInputs.size(); i++)
    {
        const CMWInput& input = block.body.vInputs[i];
        map<Commitment, CMWOwnedOutput>::iterator it = mapOwnedOutputs.find(input.commitment);
        if (it != mapOwnedOutputs.end() && !it->second.fSpent)
        {
            it->second.fSpent = true;
            printf("CMWWallet::ScanForOutputs() : our MW output spent at height %d\n", block.nHeight);
        }
    }

    if (nFound > 0)
        SaveToWalletDB();

    return nFound;
}

void CMWWallet::MarkOutputSpent(const Commitment& commitment)
{
    LOCK(cs_mwwallet);

    map<Commitment, CMWOwnedOutput>::iterator it = mapOwnedOutputs.find(commitment);
    if (it != mapOwnedOutputs.end())
    {
        it->second.fSpent = true;
    }
}

vector<CMWOwnedOutput> CMWWallet::GetOwnedOutputs() const
{
    LOCK(cs_mwwallet);

    vector<CMWOwnedOutput> vResult;
    for (map<Commitment, CMWOwnedOutput>::const_iterator it = mapOwnedOutputs.begin();
         it != mapOwnedOutputs.end(); ++it)
    {
        vResult.push_back(it->second);
    }
    return vResult;
}

vector<CMWOwnedOutput> CMWWallet::GetUnspentOutputs() const
{
    LOCK(cs_mwwallet);

    vector<CMWOwnedOutput> vResult;
    for (map<Commitment, CMWOwnedOutput>::const_iterator it = mapOwnedOutputs.begin();
         it != mapOwnedOutputs.end(); ++it)
    {
        if (!it->second.fSpent)
            vResult.push_back(it->second);
    }
    return vResult;
}

void CMWWallet::AddOwnedOutput(const CMWOwnedOutput& output)
{
    LOCK(cs_mwwallet);

    mapOwnedOutputs[output.output.commitment] = output;
}

size_t CMWWallet::GetOutputCount() const
{
    LOCK(cs_mwwallet);

    return mapOwnedOutputs.size();
}

bool CMWWallet::GetScanPubKey(unsigned char* pKeyOut) const
{
    LOCK(cs_mwwallet);

    if (!HasKeys())
        return false;

    memcpy(pKeyOut, scanPubKey, PUBKEY_SIZE);
    return true;
}

bool CMWWallet::GetSpendPubKey(unsigned char* pKeyOut) const
{
    LOCK(cs_mwwallet);

    if (!HasKeys())
        return false;

    memcpy(pKeyOut, spendPubKey, PUBKEY_SIZE);
    return true;
}

bool CMWWallet::SaveToWalletDB()
{
    if (!pWallet || !pWallet->fFileBacked)
        return false;

    CWalletDB walletdb(pWallet->strWalletFile);

    if (!walletdb.WriteMWKey(string("scan"), scanPrivKey))
        return false;

    if (!walletdb.WriteMWKey(string("spend"), spendPrivKey))
        return false;

    for (map<Commitment, CMWOwnedOutput>::const_iterator it = mapOwnedOutputs.begin();
         it != mapOwnedOutputs.end(); ++it)
    {
        string strCommitHex = HexStr(it->first.begin(), it->first.end());
        if (!walletdb.WriteMWOutput(strCommitHex, it->second))
            return false;
    }

    return true;
}

bool CMWWallet::LoadFromWalletDB()
{
    if (!pWallet || !pWallet->fFileBacked)
        return false;

    CWalletDB walletdb(pWallet->strWalletFile, "r");

    SecretKey loadedScanKey;
    if (walletdb.ReadMWKey(string("scan"), loadedScanKey))
    {
        scanPrivKey = loadedScanKey;
        DerivePublicKey(scanPrivKey, scanPubKey);
    }

    SecretKey loadedSpendKey;
    if (walletdb.ReadMWKey(string("spend"), loadedSpendKey))
    {
        spendPrivKey = loadedSpendKey;
        DerivePublicKey(spendPrivKey, spendPubKey);
    }

    if (HasKeys())
        printf("CMWWallet::LoadFromWalletDB() : MW keys loaded successfully\n");

    return true;
}

}

#endif
