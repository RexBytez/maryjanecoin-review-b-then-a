#include "dandelion.h"
#include "net.h"
#include "util.h"

CDandelionRouter g_dandelion;

CDandelionRouter::CDandelionRouter()
    : nEpochStart(0)
{

}

void CDandelionRouter::SelectStemPeers()
{
    vStemPeers.clear();

    std::vector<CNode*> vCandidates;
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
        {

            if (!pnode->fInbound &&
                pnode->fSuccessfullyConnected &&
                !pnode->fDisconnect)
            {
                vCandidates.push_back(pnode);
            }
        }
    }

    if (vCandidates.empty()) {

        return;
    }

    int nPick = std::min((int)vCandidates.size(), DANDELION_STEM_PEERS);
    for (int i = 0; i < nPick; ++i) {
        int64_t nRange = (int64_t)(vCandidates.size() - i);
        int     j      = i + (int)(GetRand((uint64_t)nRange));
        std::swap(vCandidates[i], vCandidates[j]);
        vStemPeers.push_back(vCandidates[i]);
    }
}

void CDandelionRouter::MaybeRotateEpoch()
{
    int64_t nNow = GetTime();
    if (nEpochStart == 0 || nNow - nEpochStart >= (int64_t)DANDELION_EPOCH_SECONDS) {
        nEpochStart = nNow;
        SelectStemPeers();

    }
}

DandelionPhase CDandelionRouter::GetPhase(const uint256& txHash)
{
    LOCK(cs_dandelion);
    MaybeRotateEpoch();

    if (setFluffedTxs.count(txHash))
        return DANDELION_FLUFF;

    std::map<uint256, CDandelionTx>::const_iterator it = mapStemTxs.find(txHash);
    if (it != mapStemTxs.end())
        return it->second.phase;

    return DANDELION_FLUFF;
}

void CDandelionRouter::AddLocalTx(const uint256& txHash)
{
    LOCK(cs_dandelion);
    MaybeRotateEpoch();

    if (setFluffedTxs.count(txHash))
        return;
    if (mapStemTxs.count(txHash))
        return;

    CDandelionTx dtx;
    dtx.hash           = txHash;
    dtx.phase          = DANDELION_STEM;
    dtx.nTimeStemStart = GetTime();
    dtx.stemHops       = 0;
    dtx.isOurTx        = true;

    mapStemTxs[txHash] = dtx;
}

void CDandelionRouter::AddStemTx(const uint256& txHash, int hops)
{
    LOCK(cs_dandelion);
    MaybeRotateEpoch();

    if (setFluffedTxs.count(txHash))
        return;

    std::map<uint256, CDandelionTx>::iterator it = mapStemTxs.find(txHash);
    if (it != mapStemTxs.end()) {

        if (hops > it->second.stemHops)
            it->second.stemHops = hops;
        return;
    }

    CDandelionTx dtx;
    dtx.hash           = txHash;
    dtx.phase          = DANDELION_STEM;
    dtx.nTimeStemStart = GetTime();
    dtx.stemHops       = hops;
    dtx.isOurTx        = false;

    mapStemTxs[txHash] = dtx;
}

void CDandelionRouter::MarkFluffed(const uint256& txHash)
{
    LOCK(cs_dandelion);

    mapStemTxs.erase(txHash);
    setFluffedTxs.insert(txHash);
}

bool CDandelionRouter::IsStemPhase(const uint256& txHash)
{
    LOCK(cs_dandelion);

    if (setFluffedTxs.count(txHash))
        return false;

    std::map<uint256, CDandelionTx>::const_iterator it = mapStemTxs.find(txHash);
    if (it != mapStemTxs.end())
        return it->second.phase == DANDELION_STEM;

    return false;
}

CNode* CDandelionRouter::GetStemPeer(const uint256& txHash)
{
    LOCK(cs_dandelion);
    MaybeRotateEpoch();

    if (vStemPeers.empty())
        return NULL;

    const unsigned char* pData = txHash.begin();
    uint64_t nSelector = 0;
    for (int i = 0; i < 8; ++i)
        nSelector = (nSelector << 8) | pData[i];

    return vStemPeers[nSelector % vStemPeers.size()];
}

std::vector<uint256> CDandelionRouter::GetEmbargoedTxs()
{
    LOCK(cs_dandelion);

    int64_t nCutoff = GetTime() - (int64_t)DANDELION_EMBARGO_SECONDS;
    std::vector<uint256> vExpired;

    for (std::map<uint256, CDandelionTx>::const_iterator it = mapStemTxs.begin();
         it != mapStemTxs.end(); ++it)
    {
        if (it->second.nTimeStemStart <= nCutoff)
            vExpired.push_back(it->first);
    }

    return vExpired;
}

bool CDandelionRouter::ShouldFluff(const uint256& txHash)
{
    LOCK(cs_dandelion);

    if (vStemPeers.empty())
        return true;

    std::map<uint256, CDandelionTx>::const_iterator it = mapStemTxs.find(txHash);
    if (it != mapStemTxs.end()) {
        if (it->second.stemHops >= DANDELION_MAX_STEM_HOPS)
            return true;
    }

    return GetRand(100) < (uint64_t)(DANDELION_FLUFF_PROBABILITY * 100.0 + 0.5);
}

int CDandelionRouter::GetStemTxCount() const
{
    LOCK(cs_dandelion);
    return (int)mapStemTxs.size();
}

int CDandelionRouter::GetFluffedTxCount() const
{
    LOCK(cs_dandelion);
    return (int)setFluffedTxs.size();
}

int CDandelionRouter::GetStemPeerCount() const
{
    LOCK(cs_dandelion);
    return (int)vStemPeers.size();
}

int64_t CDandelionRouter::GetEpochStart() const
{
    LOCK(cs_dandelion);
    return nEpochStart;
}
