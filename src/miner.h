#ifndef NOVACOIN_MINER_H
#define NOVACOIN_MINER_H

#include "main.h"
#include "wallet.h"

CBlock* CreateNewBlock(CWallet* pwallet, bool fProofOfStake=false, int64_t* pFees = 0);

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce);

void FormatHashBuffers(CBlock* pblock, char* pmidstate, char* pdata, char* phash1);

bool CheckWork(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey);

bool CheckStake(CBlock* pblock, CWallet& wallet);

void SHA256Transform(void* pstate, void* pinput, const void* pinit);

void PowMiner(CWallet *pwallet);

void StakeMiner(CWallet *pwallet);

#endif
