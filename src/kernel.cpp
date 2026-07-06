#include <boost/assign/list_of.hpp>

#include "kernel.h"
#include "txdb.h"

using namespace std;

extern unsigned int nStakeMaxAge;
extern unsigned int nTargetSpacing;

typedef std::map<int, unsigned int> MapModifierCheckpoints;

static std::map<int, unsigned int> mapStakeModifierCheckpoints =
    boost::assign::map_list_of
    ( 0, 0xfd11f4e7u )
    ;

static std::map<int, unsigned int> mapStakeModifierCheckpointsTestNet =
    boost::assign::map_list_of
    ( 0, 0xfd11f4e7u )
    ;

int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd)
{

    return min(nIntervalEnd - nIntervalBeginning - nStakeMinAge, (int64_t)nStakeMaxAge);
}

int64_t GetWeight2(int64_t nIntervalBeginning, int64_t nIntervalEnd)
{

	int64_t nStakeMinAgeV2 = nStakeMinAge;
    return min(nIntervalEnd - nIntervalBeginning - nStakeMinAgeV2, (int64_t)nStakeMaxAge);
}

static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    if (!pindex->GeneratedStakeModifier())
        return error("GetLastStakeModifier: no generation at genesis block");
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}

static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert (nSection >= 0 && nSection < 64);
    return (nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

static int64_t GetStakeModifierSelectionInterval()
{
    int64_t nSelectionInterval = 0;
    for (int nSection=0; nSection<64; nSection++)
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    return nSelectionInterval;
}

static bool SelectBlockFromCandidates(vector<pair<int64_t, uint256> >& vSortedByTimestamp, map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev, const CBlockIndex** pindexSelected)
{
    bool fSelected = false;
    uint256 hashBest = 0;
    *pindexSelected = (const CBlockIndex*) 0;
    BOOST_FOREACH(const PAIRTYPE(int64_t, uint256)& item, vSortedByTimestamp)
    {
        if (!mapBlockIndex.count(item.second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString().c_str());
        const CBlockIndex* pindex = mapBlockIndex[item.second];
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;
        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;

        uint256 hashProof = pindex->IsProofOfStake()? pindex->hashProofOfStake : pindex->GetBlockHash();
        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
        uint256 hashSelection = Hash(ss.begin(), ss.end());

        if (pindex->IsProofOfStake())
            hashSelection >>= 32;
        if (fSelected && hashSelection < hashBest)
        {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
        else if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
    }
    if (fDebug && GetBoolArg("-printstakemodifier"))
        printf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString().c_str());
    return fSelected;
}

bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;
    }

    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    if (fDebug)
    {
        printf("ComputeNextStakeModifier: prev modifier=0x%016" PRIx64 " time=%s\n", nStakeModifier, DateTimeStrFormat(nModifierTime).c_str());
    }
    if (nModifierTime / nModifierInterval >= pindexPrev->GetBlockTime() / nModifierInterval)
        return true;

    vector<pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * nModifierInterval / nTargetSpacing);
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / nModifierInterval) * nModifierInterval - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;
    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
    {
        vSortedByTimestamp.push_back(make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }
    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound=0; nRound<min(64, (int)vSortedByTimestamp.size()); nRound++)
    {

        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);

        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);

        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);

        mapSelectedBlocks.insert(make_pair(pindex->GetBlockHash(), pindex));
        if (fDebug && GetBoolArg("-printstakemodifier"))
            printf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n", nRound, DateTimeStrFormat(nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
    }

    if (fDebug && GetBoolArg("-printstakemodifier"))
    {
        string strSelectionMap = "";

        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate)
        {

            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        BOOST_FOREACH(const PAIRTYPE(uint256, const CBlockIndex*)& item, mapSelectedBlocks)
        {

            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake()? "S" : "W");
        }
        printf("ComputeNextStakeModifier: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    }
    if (fDebug)
    {
        printf("ComputeNextStakeModifier: new modifier=0x%016" PRIx64 " time=%s\n", nStakeModifierNew, DateTimeStrFormat(pindexPrev->GetBlockTime()).c_str());
    }

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

static bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    nStakeModifier = 0;
    if (!mapBlockIndex.count(hashBlockFrom))
        return error("GetKernelStakeModifier() : block not indexed");
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();
    const CBlockIndex* pindex = pindexFrom;

    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval)
    {
        if (!pindex->pnext)
        {
            if (fPrintProofOfStake || (pindex->GetBlockTime() + nStakeMinAge - nStakeModifierSelectionInterval > GetAdjustedTime()))
                return error("GetKernelStakeModifier() : reached best block %s at height %d from block %s",
                    pindex->GetBlockHash().ToString().c_str(), pindex->nHeight, hashBlockFrom.ToString().c_str());
            else
                return false;
        }
        pindex = pindex->pnext;
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

uint256 stakeHash(unsigned int nTimeTx, unsigned int nTxPrevTime, CDataStream ss, unsigned int prevoutIndex, unsigned int nTxPrevOffset, unsigned int nTimeBlockFrom)
{
	ss << nTimeBlockFrom << nTxPrevOffset << nTxPrevTime << prevoutIndex << nTimeTx;
	return Hash(ss.begin(), ss.end());
}

static int nStakeDebugCount = 0;
bool stakeTargetHit(uint256 hashProofOfStake, int64_t nTimeWeight, int64_t nValueIn, CBigNum bnTargetPerCoinDay)
{

	CBigNum bnCoinDayWeight = CBigNum(nValueIn) * nTimeWeight / COIN / (24 * 60 * 60);
	CBigNum bnTarget = bnCoinDayWeight * bnTargetPerCoinDay;

	nStakeDebugCount++;
	if (nStakeDebugCount == 1 || nStakeDebugCount == 100000)
		printf("stakeTargetHit: coinDayWeight=%s value=%" PRId64 " timeWeight=%" PRId64 " target=%s hash=%s hit=%d\n",
			bnCoinDayWeight.ToString().c_str(), nValueIn, nTimeWeight,
			bnTarget.GetHex().c_str(), hashProofOfStake.GetHex().c_str(),
			(CBigNum(hashProofOfStake) < bnTarget) ? 1 : 0);

	return (CBigNum(hashProofOfStake) < bnTarget);
}

bool CheckStakeKernelHash(unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev,
	const COutPoint& prevout, unsigned int& nTimeTx, unsigned int nHashDrift, bool fCheck, uint256& hashProofOfStake, bool fPrintProofOfStake)
{

	int64_t nValueIn = txPrev.vout[prevout.n].nValue;
	unsigned int nTxPrevTime = txPrev.nTime;
	unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();

	if (nTimeTx  < txPrev.nTime)
        return error("CheckStakeKernelHash() : nTime violation");

	if (nTimeBlockFrom + nStakeMinAge > nTimeTx)
		return error("CheckStakeKernelHash() : min age violation");

	CBigNum bnTargetPerCoinDay;
	bnTargetPerCoinDay.SetCompact(nBits);

	uint64_t nStakeModifier = 0;
	int nStakeModifierHeight = 0;
	int64_t nStakeModifierTime = 0;
	if (!GetKernelStakeModifier(blockFrom.GetHash(), nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake))
	{
		static bool bPrinted = false;
		if (!bPrinted) { printf("CheckStakeKernelHash: GetKernelStakeModifier FAILED for block %s\n", blockFrom.GetHash().GetHex().c_str()); bPrinted = true; }
		return false;
	}
	static bool bModOk = false;
	if (!bModOk) { printf("CheckStakeKernelHash: modifier OK, entering hash loop (nHashDrift=%u)\n", nHashDrift); bModOk = true; }

	CDataStream ss(SER_GETHASH, 0);
	ss << nStakeModifier;

	if(fCheck)
	{
		hashProofOfStake = stakeHash(nTimeTx, nTxPrevTime, ss, prevout.n, nTxPrevOffset, nTimeBlockFrom);
		return stakeTargetHit(hashProofOfStake, GetWeight((int64_t)nTxPrevTime, (int64_t)nTimeTx), nValueIn, bnTargetPerCoinDay);
	}

    bool fSuccess = false;
	unsigned int nTryTime = 0;
	unsigned int i;
	for(i = 0; i < (nHashDrift); i++)
	{

		nTryTime = nTimeTx + nHashDrift - i;
		hashProofOfStake = stakeHash(nTryTime, nTxPrevTime, ss, prevout.n, nTxPrevOffset, nTimeBlockFrom);

		if(!stakeTargetHit(hashProofOfStake, GetWeight((int64_t)nTxPrevTime, (int64_t)nTimeTx), nValueIn, bnTargetPerCoinDay))
			continue;

		fSuccess = true;
		nTimeTx = nTryTime;
		if (fDebug || fPrintProofOfStake)
		{
			printf("CheckStakeKernelHash() : using modifier 0x%016" PRIx64 " at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
				nStakeModifier, nStakeModifierHeight,
				DateTimeStrFormat(nStakeModifierTime).c_str(),
				mapBlockIndex[blockFrom.GetHash()]->nHeight,
				DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
			printf("CheckStakeKernelHash() : pass protocol=%s modifier=0x%016" PRIx64 " nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
				"0.3",
				nStakeModifier,
				nTimeBlockFrom, nTxPrevOffset, nTxPrevTime, prevout.n, nTryTime,
				hashProofOfStake.ToString().c_str());
		}
	}
	mapHashedBlocks.clear();
	mapHashedBlocks[nBestHeight] = GetTime();
    return fSuccess;
}

bool CheckProofOfStake(const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake)
{
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString().c_str());

    const CTxIn& txin = tx.vin[0];

    CTxDB txdb("r");
    CTransaction txPrev;
    CTxIndex txindex;
    if (!txPrev.ReadFromDisk(txdb, txin.prevout, txindex))
        return tx.DoS(1, error("CheckProofOfStake() : INFO: read txPrev failed"));

    if (!VerifySignature(txPrev, tx, 0, 0))
        return tx.DoS(100, error("CheckProofOfStake() : VerifySignature failed on coinstake %s", tx.GetHash().ToString().c_str()));

    CBlock block;
    if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
        return fDebug? error("CheckProofOfStake() : read block failed") : false;

	unsigned int nInterval = 0;
	unsigned int nTxTime = tx.nTime;
    if (!CheckStakeKernelHash(nBits, block, txindex.pos.nTxPos - txindex.pos.nBlockPos, txPrev, txin.prevout, nTxTime, nInterval, true, hashProofOfStake, fDebug))
        return tx.DoS(1, error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx.GetHash().ToString().c_str(), hashProofOfStake.ToString().c_str()));

    return true;
}

bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx)
{

    return (nTimeBlock == nTimeTx);
}

unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex)
{

    assert (pindex->pprev || pindex->nHeight == 0);

    CDataStream ss(SER_GETHASH, 0);
    if (pindex->pprev)
        ss << pindex->pprev->nStakeModifierChecksum;
    ss << pindex->nFlags << pindex->hashProofOfStake << pindex->nStakeModifier;
    uint256 hashChecksum = Hash(ss.begin(), ss.end());
    hashChecksum >>= (256 - 32);
    return hashChecksum.Get64();
}

bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum)
{
    MapModifierCheckpoints& checkpoints = (fTestNet ? mapStakeModifierCheckpointsTestNet : mapStakeModifierCheckpoints);

    if (checkpoints.count(nHeight))
    {
        if (nStakeModifierChecksum != checkpoints[nHeight])
        {
            printf("MaryJaneCoin: StakeModifier checkpoint mismatch at height %d: got 0x%08x, expected 0x%08x\n",
                   nHeight, nStakeModifierChecksum, checkpoints[nHeight]);
            return false;
        }
    }
    return true;
}
