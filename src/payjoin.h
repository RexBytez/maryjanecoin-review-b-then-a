#ifndef MARYJANECOIN_PAYJOIN_H
#define MARYJANECOIN_PAYJOIN_H

#include <string>
#include <vector>
#include <map>
#include <set>

#include "uint256.h"
#include "main.h"
#include "wallet.h"
#include "sync.h"

static const int PAYJOIN_VERSION = 1;
static const int DEFAULT_PAYJOIN_PORT = 0;
static const int PAYJOIN_MAX_RECEIVER_INPUTS = 5;
static const int PAYJOIN_MIN_CONFIRMATIONS = 1;
static const int64_t PAYJOIN_MAX_FEE_INCREASE = COIN;
static const int PAYJOIN_HTTP_TIMEOUT = 30;

enum PayJoinStatus {
    PAYJOIN_IDLE = 0,
    PAYJOIN_CREATING = 1,
    PAYJOIN_WAITING = 2,
    PAYJOIN_VALIDATING = 3,
    PAYJOIN_SIGNING = 4,
    PAYJOIN_COMPLETE = 5,
    PAYJOIN_ERROR = 6,
};

struct CPayJoinResult {
    uint256 txHash;
    int nSenderInputs;
    int nReceiverInputs;
    int nTotalInputs;
    int nOutputs;
    int64_t nAmount;
    int64_t nFeePaid;
    std::string strError;
    bool fSuccess;
    bool fPayJoinUsed;

    CPayJoinResult() : nSenderInputs(0), nReceiverInputs(0), nTotalInputs(0),
                       nOutputs(0), nAmount(0), nFeePaid(0), fSuccess(false),
                       fPayJoinUsed(false) {}
};

struct CPayJoinProposal {
    CTransaction txOriginal;
    CTransaction txProposal;
    int64_t nReceiverAmount;
    int64_t nTimestamp;
    bool fCompleted;

    CPayJoinProposal() : nReceiverAmount(0), nTimestamp(0), fCompleted(false) {}
};

class CPayJoinServer {
private:
    mutable CCriticalSection cs_payjoin;
    CWallet* pWallet;
    bool fRunning;
    int nPort;
    std::map<uint256, CPayJoinProposal> mapProposals;

    bool SelectReceiverInputs(const CTransaction& txOriginal,
                              std::set<std::pair<const CWalletTx*, unsigned int> >& setCoins,
                              int64_t& nValueIn);

    int64_t ScoreDenominationMatch(int64_t nUtxoValue,
                                   const std::vector<int64_t>& vSenderValues) const;

public:
    CPayJoinServer(CWallet* wallet);
    ~CPayJoinServer();

    bool CreateProposal(const CTransaction& txOriginal,
                        CTransaction& txProposal,
                        std::string& strError);

    bool ValidateOriginalTx(const CTransaction& tx,
                            int64_t& nReceiverAmount,
                            std::string& strError) const;

    bool AddReceiverInputs(CTransaction& tx,
                           const CTransaction& txOriginal,
                           std::string& strError);

    bool AdjustOutputs(CTransaction& tx,
                       int64_t nReceiverAmount,
                       int64_t nAddedInputValue,
                       std::string& strError);

    bool Start(int nListenPort, std::string& strError);

    void Stop();

    bool IsRunning() const;
    int GetPort() const;
    int GetProposalCount() const;
    std::vector<CPayJoinProposal> GetRecentProposals() const;
};

class CPayJoinClient {
private:
    mutable CCriticalSection cs_payjoin;
    CWallet* pWallet;
    PayJoinStatus nStatus;

    std::set<COutPoint> setOriginalInputs;

    std::map<CScript, int64_t> mapOriginalOutputs;

public:
    CPayJoinClient(CWallet* wallet);

    bool CreateOriginalTx(const std::string& strAddress,
                          int64_t nAmount,
                          CTransaction& txOriginal,
                          CReserveKey& reservekey,
                          std::string& strError);

    bool SendProposal(const std::string& strEndpoint,
                      const CTransaction& txOriginal,
                      CTransaction& txProposal,
                      std::string& strError);

    bool ValidateProposal(const CTransaction& txOriginal,
                          const CTransaction& txProposal,
                          const std::string& strReceiverAddress,
                          int64_t nAmount,
                          std::string& strError) const;

    bool SignAndBroadcast(CTransaction& txProposal,
                         const CTransaction& txOriginal,
                         CReserveKey& reservekey,
                         CPayJoinResult& result,
                         std::string& strError);

    CPayJoinResult SendPayJoin(const std::string& strAddress,
                               int64_t nAmount,
                               const std::string& strEndpoint);

    PayJoinStatus GetStatus() const;
};

extern CPayJoinServer* pPayJoinServer;

void ThreadPayJoinServer(void* parg);

struct CPayJoinHistoryEntry {
    uint256 txHash;
    int64_t nTimestamp;
    int64_t nAmount;
    bool fSender;
    int nSenderInputs;
    int nReceiverInputs;
    std::string strCounterparty;

    CPayJoinHistoryEntry() : nTimestamp(0), nAmount(0), fSender(true),
                             nSenderInputs(0), nReceiverInputs(0) {}
};

extern std::vector<CPayJoinHistoryEntry> vPayJoinHistory;
extern CCriticalSection cs_payjoinHistory;

void RecordPayJoinHistory(const CPayJoinHistoryEntry& entry);

std::vector<CPayJoinHistoryEntry> GetPayJoinHistory();

#endif
