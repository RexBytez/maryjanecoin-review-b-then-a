#ifndef MARYJANECOIN_BIP47_H
#define MARYJANECOIN_BIP47_H

#include <vector>
#include <string>
#include <map>
#include "key.h"
#include "serialize.h"
#include "uint256.h"
#include "script.h"

static const uint8_t BIP47_VERSION           = 0x01;
static const uint8_t BIP47_FEATURES          = 0x00;
static const uint8_t BIP47_BASE58_VERSION    = 0x47;
static const size_t  PAYMENT_CODE_SIZE       = 80;
static const size_t  PAYMENT_CODE_CORE_SIZE  = 68;
static const int64_t BIP47_NOTIFICATION_DUST = 10000;

static const size_t  BIP47_NOTIFICATION_SCRIPT_SIZE = 83;

class CPaymentCode
{
public:
    uint8_t nVersion;
    uint8_t nFeatures;
    CPubKey scanPubKey;
    CPubKey spendPubKey;

    std::string label;

    CPaymentCode()
        : nVersion(BIP47_VERSION), nFeatures(BIP47_FEATURES) {}

    CPaymentCode(const CPubKey& scan, const CPubKey& spend)
        : nVersion(BIP47_VERSION), nFeatures(BIP47_FEATURES),
          scanPubKey(scan), spendPubKey(spend) {}

    bool IsValid() const;

    bool Encode(std::vector<unsigned char>& vchPayload) const;

    bool Decode(const std::vector<unsigned char>& vchPayload);

    std::string ToBase58() const;

    bool FromBase58(const std::string& strEncoded);

    CKeyID GetNotificationAddress() const;

    bool DeriveReceiveAddress(const uint256& sharedSecret,
                              uint32_t nIndex,
                              CPubKey& destPubKeyOut,
                              CKeyID& destKeyIDOut) const;

    static bool DeriveSpendKey(const CKey& spendSecret,
                               const uint256& sharedSecret,
                               uint32_t nIndex,
                               CKey& destKeyOut);

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nVersion);
        READWRITE(nFeatures);
        READWRITE(scanPubKey);
        READWRITE(spendPubKey);
        READWRITE(label);
    )
};

struct CPaymentChannel
{
    CPaymentCode theirCode;
    uint256      sharedSecret;
    uint32_t     nNextSendIndex;
    uint32_t     nNextRecvIndex;
    bool         fNotificationSent;
    bool         fNotificationRecv;
    int64_t      nCreateTime;

    CPaymentChannel()
        : nNextSendIndex(0), nNextRecvIndex(0),
          fNotificationSent(false), fNotificationRecv(false),
          nCreateTime(0) {}

    IMPLEMENT_SERIALIZE
    (
        READWRITE(theirCode);
        READWRITE(sharedSecret);
        READWRITE(nNextSendIndex);
        READWRITE(nNextRecvIndex);
        READWRITE(fNotificationSent);
        READWRITE(fNotificationRecv);
        READWRITE(nCreateTime);
    )
};

bool ComputeBIP47SharedSecret(const CKey& privkey,
                              const CPubKey& pubkey,
                              uint256& sharedSecretOut);

bool ComputeBIP47AddressTweak(const uint256& sharedSecret,
                              uint32_t nIndex,
                              uint256& tweakOut);

bool DeriveChildPubKey(const CPubKey& spendPubKey,
                       const uint256& tweak,
                       CPubKey& childPubKeyOut,
                       CKeyID& childKeyIDOut);

bool DeriveChildPrivKey(const CKey& spendSecret,
                        const uint256& tweak,
                        CKey& childKeyOut);

bool BlindPaymentCode(const std::vector<unsigned char>& vchPayload,
                      const uint256& blindingSecret,
                      std::vector<unsigned char>& vchBlindedOut);

bool UnblindPaymentCode(const std::vector<unsigned char>& vchBlinded,
                        const uint256& blindingSecret,
                        std::vector<unsigned char>& vchPayloadOut);

CScript BuildNotificationScript(const std::vector<unsigned char>& vchBlindedPayload);

bool ExtractNotificationPayload(const CScript& script,
                                std::vector<unsigned char>& vchPayloadOut);

bool ComputeBlindingSecret(const CKey& senderScanPriv,
                           const CPubKey& receiverScanPub,
                           uint256& blindingSecretOut);

#endif
