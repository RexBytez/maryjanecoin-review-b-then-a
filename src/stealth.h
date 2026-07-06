#ifndef MARYJANECOIN_STEALTH_H
#define MARYJANECOIN_STEALTH_H

#include <vector>
#include <string>
#include "key.h"
#include "serialize.h"
#include "uint256.h"

class CStealthAddress
{
public:

    static const uint8_t STEALTH_VERSION = 0x28;

    CPubKey scanPubKey;

    CPubKey spendPubKey;

    std::string label;

    CStealthAddress() {}
    CStealthAddress(const CPubKey& scan, const CPubKey& spend)
        : scanPubKey(scan), spendPubKey(spend) {}

    std::string Encoded() const;

    bool SetEncoded(const std::string& addr);

    bool IsValid() const;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(scanPubKey);
        READWRITE(spendPubKey);
        READWRITE(label);
    )
};

struct CStealthPayment
{
    CPubKey ephemeralPubKey;
    CKeyID  destKeyID;
    CPubKey destPubKey;
};

bool GenerateStealthAddress(CStealthAddress& addressOut,
                            CKey& scanSecretOut,
                            CKey& spendSecretOut);

bool PrepareStealthPayment(const CStealthAddress& address,
                           CStealthPayment& paymentOut);

bool DetectStealthPayment(const CKey& scanSecret,
                          const CPubKey& ephemeralPubKey,
                          const CPubKey& spendPubKey,
                          CKey& destKeyOut);

bool ComputeStealthSharedSecret(const CKey& privkey,
                                const CPubKey& pubkey,
                                uint256& sharedSecretOut);

bool DeriveStealthDestination(const CPubKey& spendPubKey,
                               const uint256& sharedSecret,
                               CPubKey& destPubKeyOut,
                               CKeyID& destKeyIDOut);

bool DeriveStealthSpendKey(const CKey& spendSecret,
                           const uint256& sharedSecret,
                           CKey& destKeyOut);

#endif
