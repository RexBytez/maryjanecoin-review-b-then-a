#ifndef MARYJANECOIN_MW_WALLET_H
#define MARYJANECOIN_MW_WALLET_H

#ifdef ENABLE_MWEB

#include "mw/models/tx_body.h"
#include "mw/models/block.h"
#include "mw/state/mw_state.h"
#include "key.h"
#include "sync.h"

#include <map>
#include <vector>

class CWallet;

namespace mw {

struct CMWOwnedOutput
{
    CMWOutput output;
    BlindingFactor blindingFactor;
    SecretKey spendKey;
    int64_t nValue;
    int32_t nBlockHeight;
    bool fSpent;

    CMWOwnedOutput()
        : nValue(0), nBlockHeight(0), fSpent(false) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(output);
        READWRITE(blindingFactor);
        READWRITE(spendKey);
        READWRITE(nValue);
        READWRITE(nBlockHeight);
        READWRITE(fSpent);
    )
};

class CMWWallet
{
private:
    mutable CCriticalSection cs_mwwallet;

    CWallet* pWallet;

    SecretKey scanPrivKey;
    unsigned char scanPubKey[PUBKEY_SIZE];

    SecretKey spendPrivKey;
    unsigned char spendPubKey[PUBKEY_SIZE];

    std::map<Commitment, CMWOwnedOutput> mapOwnedOutputs;

    bool DeriveSpendKey(const unsigned char* pSenderPubKey,
                        SecretKey& keyOut) const;

public:
    CMWWallet() : pWallet(NULL)
    {
        memset(scanPubKey, 0, PUBKEY_SIZE);
        memset(spendPubKey, 0, PUBKEY_SIZE);
    }

    void Init(CWallet* pWalletIn);

    bool GenerateKeys();

    bool HasKeys() const;

    int64_t GetMWBalance() const;

    int64_t GetMWConfirmedBalance(int nMinDepth = 1) const;

    int64_t GetMWUnconfirmedBalance() const;

    bool CreateMWTransaction(const unsigned char* pDestScanKey,
                             const unsigned char* pDestSpendKey,
                             int64_t nAmount,
                             CMWTransaction& txOut);

    bool SelectMWOutputs(int64_t nTargetValue,
                         std::vector<CMWOwnedOutput>& vSelected,
                         int64_t& nValueOut) const;

    int ScanForOutputs(const CMWBlock& block);

    void MarkOutputSpent(const Commitment& commitment);

    std::vector<CMWOwnedOutput> GetOwnedOutputs() const;

    std::vector<CMWOwnedOutput> GetUnspentOutputs() const;

    void AddOwnedOutput(const CMWOwnedOutput& output);

    size_t GetOutputCount() const;

    bool GetScanPubKey(unsigned char* pKeyOut) const;

    bool GetSpendPubKey(unsigned char* pKeyOut) const;

    bool SaveToWalletDB();

    bool LoadFromWalletDB();
};

}

#endif
#endif
