#ifndef MaryJaneCoin_KERNEL_H
#define MaryJaneCoin_KERNEL_H

#include "main.h"

extern unsigned int nModifierInterval;

static const int MODIFIER_INTERVAL_RATIO = 3;

bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);

bool CheckStakeKernelHash(unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev,
	const COutPoint& prevout, unsigned int& nTimeTx, unsigned int nInterval, bool fCheck, uint256& hashProofOfStake, bool fPrintProofOfStake=false);
uint256 stakeHash(unsigned int nTimeTx, unsigned int nTxPrevTime, CDataStream ss, unsigned int prevoutIndex, unsigned int nTxPrevOffset, unsigned int nTimeBlockFrom);
bool stakeTargetHit(uint256 hashProofOfStake, int64_t nTimeWeight, int64_t nValueIn, CBigNum bnTargetPerCoinDay);

bool CheckProofOfStake(const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake);

bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx);

unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex);

bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum);

int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd);
int64_t GetWeight2(int64_t nIntervalBeginning, int64_t nIntervalEnd);

#endif
