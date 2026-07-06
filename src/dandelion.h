#ifndef MARYJANECOIN_DANDELION_H
#define MARYJANECOIN_DANDELION_H

#include <stdint.h>
#include <set>
#include <map>
#include <vector>
#include "uint256.h"
#include "sync.h"

class CNode;

static const int    DANDELION_EPOCH_SECONDS      = 600;
static const int    DANDELION_STEM_PEERS         = 2;
static const double DANDELION_FLUFF_PROBABILITY  = 0.10;
static const int    DANDELION_EMBARGO_SECONDS    = 30;
static const int    DANDELION_MAX_STEM_HOPS      = 10;

enum DandelionPhase {
    DANDELION_STEM  = 0,
    DANDELION_FLUFF = 1,
};

struct CDandelionTx {
    uint256        hash;
    DandelionPhase phase;
    int64_t        nTimeStemStart;
    int            stemHops;
    bool           isOurTx;

    CDandelionTx()
        : phase(DANDELION_STEM), nTimeStemStart(0), stemHops(0), isOurTx(false) {}
};

class CDandelionRouter {
private:
    mutable CCriticalSection cs_dandelion;

    int64_t nEpochStart;

    std::vector<CNode*> vStemPeers;

    std::map<uint256, CDandelionTx> mapStemTxs;

    std::set<uint256> setFluffedTxs;

    void SelectStemPeers();

    void MaybeRotateEpoch();

public:
    CDandelionRouter();

    DandelionPhase GetPhase(const uint256& txHash);

    void AddLocalTx(const uint256& txHash);

    void AddStemTx(const uint256& txHash, int hops);

    void MarkFluffed(const uint256& txHash);

    bool IsStemPhase(const uint256& txHash);

    CNode* GetStemPeer(const uint256& txHash);

    std::vector<uint256> GetEmbargoedTxs();

    bool ShouldFluff(const uint256& txHash);

    int     GetStemTxCount()    const;
    int     GetFluffedTxCount() const;
    int     GetStemPeerCount()  const;
    int64_t GetEpochStart()     const;
};

extern CDandelionRouter g_dandelion;

#endif
