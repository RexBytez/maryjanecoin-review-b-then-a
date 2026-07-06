#ifndef MARYJANECOIN_COINJOIN_H
#define MARYJANECOIN_COINJOIN_H

#include <vector>
#include <set>
#include "uint256.h"
#include "main.h"
#include "wallet.h"
#include "sync.h"

static const int64_t COINJOIN_DENOMINATIONS[] = {
    100000000000LL,
    10000000000LL,
    1000000000LL,
    100000000LL,
};
static const int COINJOIN_NUM_DENOMINATIONS = 4;

static const int COINJOIN_MAX_OUTPUTS = 20;

static const int COINJOIN_MIN_OUTPUTS = 3;

enum CoinJoinStatus {
    COINJOIN_IDLE = 0,
    COINJOIN_SELECTING = 1,
    COINJOIN_MIXING = 2,
    COINJOIN_COMPLETE = 3,
    COINJOIN_ERROR = 4,
};

struct CCoinJoinResult {
    uint256 txHash;
    int numInputs;
    int numOutputs;
    int64_t denomination;
    int64_t totalMixed;
    int64_t feePaid;
    std::string error;
    bool success;

    CCoinJoinResult() : numInputs(0), numOutputs(0), denomination(0), totalMixed(0), feePaid(0), success(false) {}
};

class CCoinJoinMixer {
private:
    mutable CCriticalSection cs_coinjoin;
    CWallet* pWallet;
    CoinJoinStatus nStatus;

    int64_t FindBestDenomination(int64_t nAmount);

    bool SelectMixInputs(int64_t nDenomination, int nMinOutputs,
                         std::set<std::pair<const CWalletTx*, unsigned int> >& setCoins,
                         int64_t& nValueIn);

public:
    CCoinJoinMixer(CWallet* wallet);

    CCoinJoinResult MixAmount(int64_t nAmount);

    CCoinJoinResult AutoMix();

    CoinJoinStatus GetStatus() const;

    std::vector<std::pair<int64_t, int> > GetAvailableMixes();
};

#endif
