#include "txdb.h"
#include "miner.h"
#include "kernel.h"

using namespace std;

extern unsigned int nMinerSleep;

int static FormatHashBlocks(void* pbuffer, unsigned int len)
{
    unsigned char* pdata = (unsigned char*)pbuffer;
    unsigned int blocks = 1 + ((len + 8) / 64);
    unsigned char* pend = pdata + 64 * blocks;
    memset(pdata + len, 0, 64 * blocks - len);
    pdata[len] = 0x80;
    unsigned int bits = len * 8;
    pend[-1] = (bits >> 0) & 0xff;
    pend[-2] = (bits >> 8) & 0xff;
    pend[-3] = (bits >> 16) & 0xff;
    pend[-4] = (bits >> 24) & 0xff;
    return blocks;
}

static const unsigned int pSHA256InitState[8] =
{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

void SHA256Transform(void* pstate, void* pinput, const void* pinit)
{
    SHA256_CTX ctx;
    unsigned char data[64];

    SHA256_Init(&ctx);

    for (int i = 0; i < 16; i++)
        ((uint32_t*)data)[i] = ByteReverse(((uint32_t*)pinput)[i]);

    for (int i = 0; i < 8; i++)
        ctx.h[i] = ((uint32_t*)pinit)[i];

    SHA256_Update(&ctx, data, sizeof(data));
    for (int i = 0; i < 8; i++)
        ((uint32_t*)pstate)[i] = ctx.h[i];
}

class COrphan
{
public:
    CTransaction* ptx;
    set<uint256> setDependsOn;
    double dPriority;
    double dFeePerKb;

    COrphan(CTransaction* ptxIn)
    {
        ptx = ptxIn;
        dPriority = dFeePerKb = 0;
    }

    void print() const
    {
        printf("COrphan(hash=%s, dPriority=%.1f, dFeePerKb=%.1f)\n",
               ptx->GetHash().ToString().substr(0,10).c_str(), dPriority, dFeePerKb);
        BOOST_FOREACH(uint256 hash, setDependsOn)
            printf("   setDependsOn %s\n", hash.ToString().substr(0,10).c_str());
    }
};

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;
int64_t nLastCoinStakeSearchInterval = 0;

typedef boost::tuple<double, double, CTransaction*> TxPriority;
class TxPriorityCompare
{
    bool byFee;
public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) { }
    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee)
        {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        }
        else
        {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

CBlock* CreateNewBlock(CWallet* pwallet, bool fProofOfStake, int64_t* pFees)
{
    bool fAllowChainSpending = false;

    unique_ptr<CBlock> pblock(new CBlock());
    if (!pblock.get())
        return NULL;

    CBlockIndex* pindexPrev = pindexBest;

    CTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vout.resize(1);

    if (!fProofOfStake)
    {
        CReserveKey reservekey(pwallet);
        txNew.vout[0].scriptPubKey.SetDestination(reservekey.GetReservedKey().GetID());
    }
    else
    {

        txNew.vin[0].scriptSig = (CScript() << pindexPrev->nHeight+1) + COINBASE_FLAGS;
        assert(txNew.vin[0].scriptSig.size() <= 100);

        txNew.vout[0].SetEmpty();
        txNew.vout.push_back(CTxOut());
        txNew.vout[1].SetEmpty();
    }

    pblock->vtx.push_back(txNew);

    unsigned int nBlockMaxSize = GetArg("-blockmaxsize", MAX_BLOCK_SIZE_GEN/2);

    nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(MAX_BLOCK_SIZE-1000), nBlockMaxSize));

    unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", 27000);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    unsigned int nBlockMinSize = GetArg("-blockminsize", 0);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

    int64_t nMinTxFee = MIN_TX_FEE;
    if (mapArgs.count("-mintxfee"))
        ParseMoney(mapArgs["-mintxfee"], nMinTxFee);

    pblock->nBits = GetNextTargetRequired(pindexPrev, fProofOfStake);

    int64_t nFees = 0;
    {
        LOCK2(cs_main, mempool.cs);
        CTxDB txdb("r");

        list<COrphan> vOrphan;
        map<uint256, vector<COrphan*> > mapDependers;

        vector<TxPriority> vecPriority;
        vecPriority.reserve(mempool.mapTx.size());
        for (map<uint256, CTransaction>::iterator mi = mempool.mapTx.begin(); mi != mempool.mapTx.end(); ++mi)
        {
            CTransaction& tx = (*mi).second;
            if (tx.IsCoinBase() || tx.IsCoinStake() || !tx.IsFinal())
                continue;

            COrphan* porphan = NULL;
            double dPriority = 0;
            int64_t nTotalIn = 0;
            bool fMissingInputs = false;
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
            {

                CTransaction txPrev;
                CTxIndex txindex;
                if (!txPrev.ReadFromDisk(txdb, txin.prevout, txindex))
                {
                    if (fAllowChainSpending) {

                        if (!mempool.mapTx.count(txin.prevout.hash))
                        {
                            printf("ERROR: mempool transaction missing input\n");
                            if (fDebug) assert("mempool transaction missing input" == 0);
                            fMissingInputs = true;
                            if (porphan)
                                vOrphan.pop_back();
                            break;
                        }

                        if (!porphan)
                        {

                            vOrphan.push_back(COrphan(&tx));
                            porphan = &vOrphan.back();
                        }
                        mapDependers[txin.prevout.hash].push_back(porphan);
                        porphan->setDependsOn.insert(txin.prevout.hash);
                        nTotalIn += mempool.mapTx[txin.prevout.hash].vout[txin.prevout.n].nValue;
                        continue;
                    }
                    else {
                        fMissingInputs = true;
                        continue;
                    }
                }
                int64_t nValueIn = txPrev.vout[txin.prevout.n].nValue;
                nTotalIn += nValueIn;

                int nConf = txindex.GetDepthInMainChain();
                dPriority += (double)nValueIn * nConf;
            }
            if (fMissingInputs) continue;

            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            dPriority /= nTxSize;

            double dFeePerKb =  double(nTotalIn-tx.GetValueOut()) / (double(nTxSize)/1000.0);

            if (fAllowChainSpending && porphan)
            {
                porphan->dPriority = dPriority;
                porphan->dFeePerKb = dFeePerKb;
            }
            else
                vecPriority.push_back(TxPriority(dPriority, dFeePerKb, &(*mi).second));
        }

        map<uint256, CTxIndex> mapTestPool;
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;
        bool fSortedByFee = (nBlockPrioritySize <= 0);

        TxPriorityCompare comparer(fSortedByFee);
        std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);

        while (!vecPriority.empty())
        {

            double dPriority = vecPriority.front().get<0>();
            double dFeePerKb = vecPriority.front().get<1>();
            CTransaction& tx = *(vecPriority.front().get<2>());

            std::pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
            vecPriority.pop_back();

            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            unsigned int nTxSigOps = tx.GetLegacySigOpCount();
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            if (tx.nTime > GetAdjustedTime() || (fProofOfStake && tx.nTime > pblock->vtx[0].nTime))
                continue;

            int nHeight = pindexPrev->nHeight + 1;
            int64_t nMinFee = tx.GetMinFee(nBlockSize, GMF_BLOCK, 0);

            if (fSortedByFee && (dFeePerKb < nMinTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
                continue;

            if (!fSortedByFee &&
                ((nBlockSize + nTxSize >= nBlockPrioritySize) || (dPriority < COIN * 144 / 250)))
            {
                fSortedByFee = true;
                comparer = TxPriorityCompare(fSortedByFee);
                std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
            }

            map<uint256, CTxIndex> mapTestPoolTmp(mapTestPool);
            MapPrevTx mapInputs;
            bool fInvalid;
            if (!tx.FetchInputs(txdb, mapTestPoolTmp, false, true, mapInputs, fInvalid))
                continue;

            int64_t nTxFees = tx.GetValueMapIn(mapInputs)-tx.GetValueOut();
            if (nTxFees < nMinFee)
                continue;

            nTxSigOps += tx.GetP2SHSigOpCount(mapInputs);
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            if (!tx.ConnectInputs(txdb, mapInputs, mapTestPoolTmp, CDiskTxPos(1,1,1), pindexPrev, false, true))
                continue;
            mapTestPoolTmp[tx.GetHash()] = CTxIndex(CDiskTxPos(1,1,1), tx.vout.size());
            swap(mapTestPool, mapTestPoolTmp);

            pblock->vtx.push_back(tx);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;

            if (fDebug && GetBoolArg("-printpriority"))
            {
                printf("priority %.1f feeperkb %.1f txid %s\n",
                       dPriority, dFeePerKb, tx.GetHash().ToString().c_str());
            }

            uint256 hash = tx.GetHash();
            if (mapDependers.count(hash))
            {
                BOOST_FOREACH(COrphan* porphan, mapDependers[hash])
                {
                    if (!porphan->setDependsOn.empty())
                    {
                        porphan->setDependsOn.erase(hash);
                        if (porphan->setDependsOn.empty())
                        {
                            vecPriority.push_back(TxPriority(porphan->dPriority, porphan->dFeePerKb, porphan->ptx));
                            std::push_heap(vecPriority.begin(), vecPriority.end(), comparer);
                        }
                    }
                }
            }
        }

        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;

        if (fDebug && GetBoolArg("-printpriority"))
            printf("CreateNewBlock(): total size %" PRIu64 "\n", nBlockSize);

        if (!fProofOfStake)
        {
            pblock->vtx[0].vout[0].nValue = GetProofOfWorkReward(pindexBest->nHeight, nFees);
        }

        if (pFees)
            *pFees = nFees;

        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();

        pblock->nTime          = max(pblock->vtx[0].nTime, (unsigned int)(pindexPrev->GetBlockTime() + 1));
        pblock->vtx[0].nTime   = pblock->nTime;
        pblock->nNonce         = 0;

        pblock->nVersion = CBlockHeader::CURRENT_VERSION;
    }

    return pblock.release();
}

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{

    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;

    unsigned int nHeight = pindexPrev->nHeight+1;
    pblock->vtx[0].vin[0].scriptSig = (CScript() << nHeight << CBigNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(pblock->vtx[0].vin[0].scriptSig.size() <= 100);

    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

void FormatHashBuffers(CBlock* pblock, char* pmidstate, char* pdata, char* phash1)
{

    struct
    {
        struct unnamed2
        {
            int nVersion;
            uint256 hashPrevBlock;
            uint256 hashMerkleRoot;
            unsigned int nTime;
            unsigned int nBits;
            unsigned int nNonce;
        }
        block;
        unsigned char pchPadding0[64];
        uint256 hash1;
        unsigned char pchPadding1[64];
    }
    tmp;
    memset(&tmp, 0, sizeof(tmp));

    tmp.block.nVersion       = pblock->nVersion;
    tmp.block.hashPrevBlock  = pblock->hashPrevBlock;
    tmp.block.hashMerkleRoot = pblock->hashMerkleRoot;
    tmp.block.nTime          = pblock->nTime;
    tmp.block.nBits          = pblock->nBits;
    tmp.block.nNonce         = pblock->nNonce;

    FormatHashBlocks(&tmp.block, sizeof(tmp.block));
    FormatHashBlocks(&tmp.hash1, sizeof(tmp.hash1));

    for (unsigned int i = 0; i < sizeof(tmp)/4; i++)
        ((unsigned int*)&tmp)[i] = ByteReverse(((unsigned int*)&tmp)[i]);

    SHA256Transform(pmidstate, &tmp.block, pSHA256InitState);

    memcpy(pdata, &tmp.block, 128);
    memcpy(phash1, &tmp.hash1, 64);
}

bool CheckWork(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey)
{
    uint256 hashBlock = pblock->GetHash();
    uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

    if(!pblock->IsProofOfWork())
        return error("CheckWork() : %s is not a proof-of-work block", hashBlock.GetHex().c_str());

    if (hashBlock > hashTarget)
        return error("CheckWork() : proof-of-work not meeting target");

    printf("CheckWork() : new proof-of-work block found  \n  hash: %s  \ntarget: %s\n", hashBlock.GetHex().c_str(), hashTarget.GetHex().c_str());
    pblock->print();
    printf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue).c_str());

    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != hashBestChain)
            return error("CheckWork() : generated block is stale");

        reservekey.KeepKey();

        {
            LOCK(wallet.cs_wallet);
            wallet.mapRequestCount[hashBlock] = 0;
        }

        if (!ProcessBlock(NULL, pblock))
            return error("CheckWork() : ProcessBlock, block not accepted");
    }

    return true;
}

bool CheckStake(CBlock* pblock, CWallet& wallet)
{
    uint256 proofHash = 0;
    uint256 hashBlock = pblock->GetHash();

    if(!pblock->IsProofOfStake())
        return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex().c_str());

    if (!CheckProofOfStake(pblock->vtx[1], pblock->nBits, proofHash))
        return error("CheckStake() : proof-of-stake checking failed");

    printf("CheckStake() : new proof-of-stake block found  \n  hash: %s \nproofhash: %s \n", hashBlock.GetHex().c_str(), proofHash.GetHex().c_str());
    pblock->print();
    printf("out %s\n", FormatMoney(pblock->vtx[1].GetValueOut()).c_str());

    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != hashBestChain)
            return error("CheckStake() : generated block is stale");

        {
            LOCK(wallet.cs_wallet);
            wallet.mapRequestCount[hashBlock] = 0;
        }

        if (!ProcessBlock(NULL, pblock))
            return error("CheckStake() : ProcessBlock, block not accepted");
    }

    return true;
}

void StakeMiner(CWallet *pwallet)
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    RenameThread("MaryJaneCoin-miner");

    bool fTryToSync = true;

    while (true)
    {
        if (fShutdown)
            return;

        while (pwallet->IsLocked())
        {
            nLastCoinStakeSearchInterval = 0;
            MilliSleep(1000);
            if (fShutdown)
                return;
        }

        if (vNodes.empty())
            printf("StakeMiner: waiting for peers...\n");
        else if (IsInitialBlockDownload())
            printf("StakeMiner: initial block download...\n");
        else if (!pwallet->MintableCoins())
            printf("StakeMiner: no mintable coins...\n");

        while (vNodes.empty() || IsInitialBlockDownload() || !pwallet->MintableCoins())
        {
            nLastCoinStakeSearchInterval = 0;
            MilliSleep(1000);
            if (fShutdown)
                return;
        }
        printf("StakeMiner: ready to stake! MintableCoins=true\n");

        fTryToSync = false;

        int64_t nFees;
        unique_ptr<CBlock> pblock(CreateNewBlock(pwallet, true, &nFees));
        if (!pblock.get())
        {
            printf("StakeMiner: CreateNewBlock failed\n");
            MilliSleep(nMinerSleep);
            continue;
        }
        printf("StakeMiner: block created, attempting SignBlock...\n");

        if (pblock->SignBlock(*pwallet, nFees))
        {
            printf("StakeMiner: *** PoS BLOCK SIGNED! ***\n");
            SetThreadPriority(THREAD_PRIORITY_NORMAL);
            CheckStake(pblock.get(), *pwallet);
            SetThreadPriority(THREAD_PRIORITY_LOWEST);
            MilliSleep(500);
        }
        else
            MilliSleep(nMinerSleep);
    }
}

static int nPowThreads = 0;
static bool fBlockFound = false;

void PowMinerThread(CWallet *pwallet, int threadId, int totalThreads)
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    char threadName[32];
    sprintf(threadName, "pow-miner-%d", threadId);
    RenameThread(threadName);

    CReserveKey reservekey(pwallet);
    unsigned int nExtraNonce = threadId;

    while (true)
    {
        if (fShutdown)
            return;

        if (nBestHeight >= LAST_POW_BLOCK)
        {
            if (threadId == 0)
                printf("MaryJaneCoin PoW miner: reached LAST_POW_BLOCK, stopping\n");
            return;
        }

        fBlockFound = false;

        auto_ptr<CBlock> pblock(CreateNewBlock(pwallet, false));
        if (!pblock.get())
        {
            MilliSleep(1000);
            continue;
        }

        IncrementExtraNonce(pblock.get(), pindexBest, nExtraNonce);

        pblock->nNonce = (unsigned int)(threadId * (0xFFFFFFFF / totalThreads));

        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

        if (threadId == 0)
            printf("MaryJaneCoin PoW miner: %d threads mining block %d\n", totalThreads, nBestHeight + 1);

        while (true)
        {
            if (fShutdown || fBlockFound)
                break;

            if (pblock->hashPrevBlock != hashBestChain)
                break;

            uint256 hash = pblock->GetHash();
            if (hash <= hashTarget)
            {
                fBlockFound = true;
                printf("MaryJaneCoin PoW miner [thread %d]: BLOCK FOUND! hash: %s\n", threadId, hash.GetHex().c_str());
                SetThreadPriority(THREAD_PRIORITY_NORMAL);
                CheckWork(pblock.get(), *pwallet, reservekey);
                SetThreadPriority(THREAD_PRIORITY_LOWEST);
                break;
            }

            pblock->nNonce++;
            if (pblock->nNonce == 0)
                break;
        }

        MilliSleep(50);
    }
}

void PowMiner(CWallet *pwallet)
{

    nPowThreads = GetArg("-genproclimit", boost::thread::hardware_concurrency());
    if (nPowThreads <= 0)
        nPowThreads = 1;

    printf("MaryJaneCoin PoW miner starting with %d threads\n", nPowThreads);

    if (nPowThreads == 1)
    {
        PowMinerThread(pwallet, 0, 1);
    }
    else
    {
        boost::thread_group minerThreads;
        for (int i = 1; i < nPowThreads; i++)
            minerThreads.create_thread(boost::bind(&PowMinerThread, pwallet, i, nPowThreads));

        PowMinerThread(pwallet, 0, nPowThreads);
        minerThreads.join_all();
    }
}
