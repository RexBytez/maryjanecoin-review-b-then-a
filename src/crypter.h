#ifndef __CRYPTER_H__
#define __CRYPTER_H__

#include "support/allocators/secure.h"
#include "key.h"
#include "serialize.h"

const unsigned int WALLET_CRYPTO_KEY_SIZE = 32;
const unsigned int WALLET_CRYPTO_SALT_SIZE = 8;
const unsigned int WALLET_CRYPTO_IV_SIZE = 16;

class CMasterKey
{
public:
    std::vector<unsigned char> vchCryptedKey;
    std::vector<unsigned char> vchSalt;

    unsigned int nDerivationMethod;
    unsigned int nDeriveIterations;

    std::vector<unsigned char> vchOtherDerivationParameters;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(vchCryptedKey);
        READWRITE(vchSalt);
        READWRITE(nDerivationMethod);
        READWRITE(nDeriveIterations);
        READWRITE(vchOtherDerivationParameters);
    )
    CMasterKey()
    {

        nDeriveIterations = 25000;
        nDerivationMethod = 1;
        vchOtherDerivationParameters = std::vector<unsigned char>(0);
    }

    CMasterKey(unsigned int nDerivationMethodIndex)
    {
        switch (nDerivationMethodIndex)
        {
            case 0:
            default:
                nDeriveIterations = 25000;
                nDerivationMethod = 0;
                vchOtherDerivationParameters = std::vector<unsigned char>(0);
            break;

            case 1:
                nDeriveIterations = 10000;
                nDerivationMethod = 1;
                vchOtherDerivationParameters = std::vector<unsigned char>(0);
            break;
        }
    }

};

typedef std::vector<unsigned char, secure_allocator<unsigned char> > CKeyingMaterial;

class CCrypter
{
private:
    std::vector<unsigned char, secure_allocator<unsigned char>> vchKey;
    std::vector<unsigned char, secure_allocator<unsigned char>> vchIV;
    bool fKeySet;

public:
    bool SetKeyFromPassphrase(const SecureString &strKeyData, const std::vector<unsigned char>& chSalt, const unsigned int nRounds, const unsigned int nDerivationMethod);
    bool Encrypt(const CKeyingMaterial& vchPlaintext, std::vector<unsigned char> &vchCiphertext);
    bool Decrypt(const std::vector<unsigned char>& vchCiphertext, CKeyingMaterial& vchPlaintext);
    bool SetKey(const CKeyingMaterial& chNewKey, const std::vector<unsigned char, secure_allocator<unsigned char>>& chNewIV);

    void CleanKey()
    {
        if (!vchKey.empty()) {
            memory_cleanse(vchKey.data(), vchKey.size());
        }
        if (!vchIV.empty()) {
            memory_cleanse(vchIV.data(), vchIV.size());
        }
        fKeySet = false;
    }

    CCrypter()
    {
        fKeySet = false;

        vchKey.resize(WALLET_CRYPTO_KEY_SIZE);
        vchIV.resize(WALLET_CRYPTO_IV_SIZE);
    }

    ~CCrypter()
    {
        CleanKey();
    }
};

bool EncryptSecret(const CKeyingMaterial& vMasterKey, const CSecret &vchPlaintext, const uint256& nIV, std::vector<unsigned char> &vchCiphertext);
bool DecryptSecret(const CKeyingMaterial& vMasterKey, const std::vector<unsigned char> &vchCiphertext, const uint256& nIV, CSecret &vchPlaintext);

#endif
