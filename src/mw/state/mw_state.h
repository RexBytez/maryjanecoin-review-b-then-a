#ifndef MARYJANECOIN_MW_STATE_MW_STATE_H
#define MARYJANECOIN_MW_STATE_MW_STATE_H

#ifdef ENABLE_MWEB

#include "mw/models/output.h"
#include "mw/models/kernel.h"
#include "mw/models/block.h"
#include "mw/state/mmr.h"
#include "sync.h"

#include <map>
#include <set>
#include <deque>
#include <string>
#include <stdint.h>

namespace mw {

struct CMWBlockUndo
{
    std::vector<Commitment> vAddedOutputs;
    std::vector<CMWOutput>  vSpentOutputs;
    uint64_t nMMRLeavesBefore;
    uint64_t nKernelsBefore;
    int64_t  nSupplyDelta;
    uint256  hashPrevMWBlock;
    int32_t  nPrevHeight;

    CMWBlockUndo()
        : nMMRLeavesBefore(0), nKernelsBefore(0), nSupplyDelta(0),
          hashPrevMWBlock(0), nPrevHeight(0) {}

    IMPLEMENT_SERIALIZE(
        READWRITE(vAddedOutputs);
        READWRITE(vSpentOutputs);
        READWRITE(nMMRLeavesBefore);
        READWRITE(nKernelsBefore);
        READWRITE(nSupplyDelta);
        READWRITE(hashPrevMWBlock);
        READWRITE(nPrevHeight);
    )
};

static const size_t MW_UNDO_WINDOW = 1000;

class CMWState
{
private:
    mutable CCriticalSection cs_mwstate;

    std::map<Commitment, CMWOutput> mapOutputs;

    std::set<Commitment> setSpent;

    CMMR mmr;

    std::map<Commitment, uint64_t> mapMMRIndex;

    std::vector<Commitment> vKernelExcesses;

    uint256 hashLatestMWBlock;

    int32_t nHeight;

    int64_t nMWEBSupply;

    std::map<uint256, CMWBlockUndo> mapBlockUndo;
    std::deque<uint256> dqUndoOrder;

public:
    CMWState()
    {
        hashLatestMWBlock = 0;
        nHeight = 0;
        nMWEBSupply = 0;
    }

    ~CMWState();

    bool AddOutput(const CMWOutput& output);

    bool SpendOutput(const Commitment& commitment);

    bool HasOutput(const Commitment& commitment) const;

    bool GetOutput(const Commitment& commitment, CMWOutput& outputOut) const;

    size_t GetOutputCount() const;

    std::vector<Commitment> GetAllCommitments() const;

    uint256 GetMMRRoot() const;

    CMMRProof GetOutputProof(const Commitment& commitment) const;

    bool VerifyOutputProof(const Commitment& commitment, const CMMRProof& proof) const;

    void AddKernelExcess(const Commitment& excess);

    const std::vector<Commitment>& GetKernelExcesses() const { return vKernelExcesses; }

    uint256 GetLatestMWBlockHash() const;

    int32_t GetHeight() const;

    void SetLatestBlock(const uint256& hash, int32_t height);

    int64_t GetMWEBSupply() const;

    void AdjustSupply(int64_t nDelta);

    bool ApplyBlock(const mw::CMWBlock& mwBlock, int32_t nBlockHeight,
                    const uint256& hashBlock, int64_t nSupplyDelta);

    bool DisconnectBlock(const uint256& hashBlock, bool fNoMWChange);

    bool HasUndo(const uint256& hashBlock) const;

    size_t GetUndoWindowSize() const;

    uint256 ComputeStateHash() const;

    void Snapshot(CMWState& snapshot) const;

    void Restore(const CMWState& snapshot);

    void Clear();

    bool SaveToDB(const std::string& strPath) const;

    bool LoadFromDB(const std::string& strPath);
};

}

#endif
#endif
