#include "main.h"
#include "bitcoinrpc.h"
#include "util.h"
#include "txdb.h"
#include "wallet.h"

using namespace json_spirit;
using namespace std;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);
extern enum Checkpoints::CPMode CheckpointsMode;
extern CWallet* pwalletMain;

double GetDifficulty(const CBlockIndex* blockindex)
{

    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return 1.0;
        else
            blockindex = GetLastBlockIndex(pindexBest, false);
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

double GetPoWMHashPS()
{
    if (pindexBest->nHeight >= LAST_POW_BLOCK)
        return 0;

    int nPoWInterval = 72;
    int64_t nTargetSpacingWorkMin = 30, nTargetSpacingWork = 30;

    CBlockIndex* pindex = pindexGenesisBlock;
    CBlockIndex* pindexPrevWork = pindexGenesisBlock;

    while (pindex)
    {
        if (pindex->IsProofOfWork())
        {
            int64_t nActualSpacingWork = pindex->GetBlockTime() - pindexPrevWork->GetBlockTime();
            nTargetSpacingWork = ((nPoWInterval - 1) * nTargetSpacingWork + nActualSpacingWork + nActualSpacingWork) / (nPoWInterval + 1);
            nTargetSpacingWork = max(nTargetSpacingWork, nTargetSpacingWorkMin);
            pindexPrevWork = pindex;
        }

        pindex = pindex->pnext;
    }

    return GetDifficulty() * 4294.967296 / nTargetSpacingWork;
}

double GetPoSKernelPS()
{
    int nPoSInterval = 72;
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    CBlockIndex* pindex = pindexBest;;
    CBlockIndex* pindexPrevStake = NULL;

    while (pindex && nStakesHandled < nPoSInterval)
    {
        if (pindex->IsProofOfStake())
        {
            dStakeKernelsTriedAvg += GetDifficulty(pindex) * 4294967296.0;
            nStakesTime += pindexPrevStake ? (pindexPrevStake->nTime - pindex->nTime) : 0;
            pindexPrevStake = pindex;
            nStakesHandled++;
        }

        pindex = pindex->pprev;
    }

    return nStakesTime ? dStakeKernelsTriedAvg / nStakesTime : 0;
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool fPrintTransactionDetail)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
    result.push_back(Pair("time", (boost::int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));
    result.push_back(Pair("blocktrust", leftTrim(blockindex->GetBlockTrust().GetHex(), '0')));
    result.push_back(Pair("chaintrust", leftTrim(blockindex->nChainTrust.GetHex(), '0')));
    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));

    result.push_back(Pair("flags", strprintf("%s%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work", blockindex->GeneratedStakeModifier()? " stake-modifier": "")));
    result.push_back(Pair("proofhash", blockindex->IsProofOfStake()? blockindex->hashProofOfStake.GetHex() : blockindex->GetBlockHash().GetHex()));
    result.push_back(Pair("entropybit", (int)blockindex->GetStakeEntropyBit()));
    result.push_back(Pair("modifier", strprintf("%016" PRIx64, blockindex->nStakeModifier)));
    result.push_back(Pair("modifierchecksum", strprintf("%08x", blockindex->nStakeModifierChecksum)));
    Array txinfo;
    BOOST_FOREACH (const CTransaction& tx, block.vtx)
    {
        if (fPrintTransactionDetail)
        {
            Object entry;

            entry.push_back(Pair("txid", tx.GetHash().GetHex()));
            TxToJSON(tx, 0, entry);

            txinfo.push_back(entry);
        }
        else
            txinfo.push_back(tx.GetHash().GetHex());
    }

    result.push_back(Pair("tx", txinfo));

    if (block.IsProofOfStake())
        result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));

    return result;
}

Value getbestblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getbestblockhash\n"
            "Returns the hash of the best block in the longest block chain.");

    return hashBestChain.GetHex();
}

Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight;
}

Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "Returns the difficulty as a multiple of the minimum difficulty.");

    Object obj;
    obj.push_back(Pair("proof-of-work",        GetDifficulty()));
    obj.push_back(Pair("proof-of-stake",       GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(Pair("search-interval",      (int)nLastCoinStakeSearchInterval));
    return obj;
}

Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1 || AmountFromValue(params[0]) < MIN_TX_FEE)
        throw runtime_error(
            "settxfee <amount>\n"
            "<amount> is a real and is rounded to the nearest 0.01");

    nTransactionFee = AmountFromValue(params[0]);
    nTransactionFee = (nTransactionFee / CENT) * CENT;

    return true;
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getrawmempool\n"
            "Returns all transaction ids in memory pool.");

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash <index>\n"
            "Returns hash of block in best-block-chain at <index>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->phashBlock->GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <hash> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

Value getblockbynumber(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <number> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-number.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;

    uint256 hash = *pblockindex->phashBlock;

    pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

Value getcheckpoint(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getcheckpoint\n"
            "Show info of synchronized checkpoint.\n");

    Object result;
    CBlockIndex* pindexCheckpoint;

    result.push_back(Pair("synccheckpoint", Checkpoints::hashSyncCheckpoint.ToString().c_str()));
    pindexCheckpoint = mapBlockIndex[Checkpoints::hashSyncCheckpoint];
    result.push_back(Pair("height", pindexCheckpoint->nHeight));
    result.push_back(Pair("timestamp", DateTimeStrFormat(pindexCheckpoint->GetBlockTime()).c_str()));

    if (CheckpointsMode == Checkpoints::STRICT)
        result.push_back(Pair("policy", "strict"));

    if (CheckpointsMode == Checkpoints::ADVISORY)
        result.push_back(Pair("policy", "advisory"));

    if (CheckpointsMode == Checkpoints::PERMISSIVE)
        result.push_back(Pair("policy", "permissive"));

    if (mapArgs.count("-checkpointkey"))
        result.push_back(Pair("checkpointmaster", true));

    return result;
}

Value createbootstrap(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "createbootstrap\n"
            "Creates a clean bootstrap.dat file containing only blocks from the best chain.\n"
            "The bootstrap file is created in the data directory with the format bootstrap.dat_<height>.\n"
            "The bootstrap file will contain blocks from genesis to the current best block.\n"
            "This creates a clean bootstrap file without fork blocks, which is faster to load.");

    LOCK(cs_main);

    if (pindexBest == NULL)
        throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Blockchain not loaded yet");

    int nCurrentHeight = nBestHeight;

    string strFileName = strprintf("bootstrap.dat_%d", nCurrentHeight);
    boost::filesystem::path filePath = GetDataDir() / strFileName;
    string strFilePath = filePath.string();

    FILE* fileCheck = fopen(strFilePath.c_str(), "rb");
    if (fileCheck != NULL)
    {
        fclose(fileCheck);
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("File already exists: %s. Delete it first.", strFilePath.c_str()));
    }

    FILE* fileRaw = fopen(strFilePath.c_str(), "wb");
    if (fileRaw == NULL)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Cannot open file for writing: %s", strFilePath.c_str()));

    CAutoFile fileOut(fileRaw, SER_DISK, CLIENT_VERSION);

    printf("Creating bootstrap file: %s\n", strFilePath.c_str());

    vector<CBlockIndex*> vBlocks;
    for (CBlockIndex* pindex = pindexBest; pindex; pindex = pindex->pprev)
        vBlocks.push_back(pindex);

    reverse(vBlocks.begin(), vBlocks.end());

    int nBlocksWritten = 0;
    int64_t nFileSize = 0;

    extern unsigned char pchMessageStart[4];

    BOOST_FOREACH(CBlockIndex* pindex, vBlocks)
    {

        CBlock block;
        if (!block.ReadFromDisk(pindex))
        {
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Failed to read block %d from disk", pindex->nHeight));
        }

        unsigned int nSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);

        fileOut << FLATDATA(pchMessageStart);
        fileOut << nSize;
        fileOut << block;

        if (!fileOut.good())
        {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to write block to file");
        }

        nFileSize += sizeof(pchMessageStart) + sizeof(nSize) + nSize;
        nBlocksWritten++;

        if (nBlocksWritten % 10000 == 0)
        {
            printf("Written %d blocks...\n", nBlocksWritten);
            FileCommit(fileRaw);
        }
    }

    FileCommit(fileRaw);

    printf("Bootstrap file created: %s\n", strFilePath.c_str());
    printf("Total blocks written: %d\n", nBlocksWritten);
    printf("File size: %" PRId64 " bytes (%.2f MB)\n", nFileSize, nFileSize / (1024.0 * 1024.0));

    Object result;
    result.push_back(Pair("filepath", strFilePath));
    result.push_back(Pair("blocks", nBlocksWritten));
    result.push_back(Pair("size_bytes", nFileSize));
    result.push_back(Pair("size_mb", nFileSize / (1024.0 * 1024.0)));
    result.push_back(Pair("height_from", 0));
    result.push_back(Pair("height_to", nBestHeight));

    return result;
}

Value reorganizetoheight(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "reorganizetoheight <height>\n"
            "Reorganizes the blockchain back to the specified block height.\n"
            "This will disconnect all blocks above the specified height from the best chain.\n"
            "WARNING: This is a dangerous operation. Use with caution!");

    LOCK(cs_main);

    int nTargetHeight = params[0].get_int();

    if (nTargetHeight < 0 || nTargetHeight > nBestHeight)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Target height must be between 0 and current best height");

    if (nTargetHeight == nBestHeight)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Already at target height");

    CBlockIndex* pindexTarget = pindexBest;
    while (pindexTarget && pindexTarget->nHeight > nTargetHeight)
        pindexTarget = pindexTarget->pprev;

    if (!pindexTarget || pindexTarget->nHeight != nTargetHeight)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Target block not found in best chain");

    bool fStakingEnabled = GetBoolArg("-staking", true);

    if (fStakingEnabled && pwalletMain)
    {
        printf("REORGANIZE: Checking staking status...\n");

        int64_t nInitialStakeInterval = nLastCoinStakeSearchInterval;
        bool fWalletUnlocked = !pwalletMain->IsLocked();

        if (nInitialStakeInterval > 0 && fWalletUnlocked)
        {
            printf("REORGANIZE: Staking appears active (interval=%d, wallet unlocked). Waiting for safety...\n",
                   (int)nInitialStakeInterval);

            MilliSleep(1000);

            int64_t nFinalStakeInterval = nLastCoinStakeSearchInterval;
            printf("REORGANIZE: Staking check complete (interval: %d -> %d). Proceeding with reorganization.\n",
                   (int)nInitialStakeInterval, (int)nFinalStakeInterval);
            printf("REORGANIZE: Note: Staking thread will be blocked during reorganization by cs_main lock.\n");
        }
        else
        {
            printf("REORGANIZE: Staking is not active (enabled=%d, locked=%d, interval=%d)\n",
                   fStakingEnabled ? 1 : 0,
                   fWalletUnlocked ? 0 : 1,
                   (int)nInitialStakeInterval);
        }
    }
    else
    {
        if (!fStakingEnabled)
            printf("REORGANIZE: Staking is disabled (via -staking=false)\n");
        else
            printf("REORGANIZE: Wallet not loaded, staking cannot be active\n");
    }

    printf("REORGANIZE: Reorganizing from height %d back to height %d\n", nBestHeight, nTargetHeight);

    CTxDB txdb("r+");
    if (!txdb.TxnBegin())
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to start database transaction");

    vector<CBlockIndex*> vDisconnect;
    for (CBlockIndex* pindex = pindexBest; pindex != pindexTarget; pindex = pindex->pprev)
        vDisconnect.push_back(pindex);

    reverse(vDisconnect.begin(), vDisconnect.end());

    BOOST_FOREACH(CBlockIndex* pindex, vDisconnect)
    {
        CBlock block;
        if (!block.ReadFromDisk(pindex))
        {
            txdb.TxnAbort();
            throw JSONRPCError(RPC_DATABASE_ERROR, strprintf("Failed to read block %d from disk", pindex->nHeight));
        }
        if (!block.DisconnectBlock(txdb, pindex))
        {
            txdb.TxnAbort();
            string strError = strprintf("Failed to disconnect block %s (height %d)",
                                       pindex->GetBlockHash().ToString().substr(0,20).c_str(),
                                       pindex->nHeight);
            strError += "\nCheck debug.log for specific transaction error.";
            strError += "\nThe issue may be a missing transaction index for a transaction referenced by this block.";
            throw JSONRPCError(RPC_DATABASE_ERROR, strError);
        }
    }

    if (!txdb.WriteHashBestChain(pindexTarget->GetBlockHash()))
    {
        txdb.TxnAbort();
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to write best chain hash");
    }

    if (!txdb.TxnCommit())
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to commit database transaction");

    BOOST_FOREACH(CBlockIndex* pindex, vDisconnect)
        if (pindex->IsProofOfStake())
            setStakeSeen.erase(make_pair(pindex->prevoutStake, pindex->nStakeTime));

    BOOST_FOREACH(CBlockIndex* pindex, vDisconnect)
        if (pindex->pprev)
            pindex->pprev->pnext = NULL;

    if (pindexTarget->pprev)
        pindexTarget->pprev->pnext = pindexTarget;

    hashBestChain = pindexTarget->GetBlockHash();
    pindexBest = pindexTarget;
    nBestHeight = pindexBest->nHeight;
    nBestChainTrust = pindexTarget->nChainTrust;
    nTimeBestReceived = GetTime();
    nTransactionsUpdated++;

    BOOST_FOREACH(PAIRTYPE(const uint256, CBlock*) item, mapOrphanBlocks)
        delete item.second;
    mapOrphanBlocks.clear();
    mapOrphanBlocksByPrev.clear();
    setStakeSeenOrphan.clear();

    mempool.clear();

    const CBlockLocator locator(pindexTarget);
    ::SetBestChain(locator);

    printf("REORGANIZE: Successfully reorganized to height %d\n", nTargetHeight);

    if (fStakingEnabled && pwalletMain)
    {
        printf("REORGANIZE: Staking will resume automatically when lock is released\n");
        printf("REORGANIZE: Staking status: enabled=%d, wallet_locked=%d\n",
               fStakingEnabled ? 1 : 0,
               pwalletMain->IsLocked() ? 1 : 0);
    }

    Object result;
    result.push_back(Pair("success", true));
    result.push_back(Pair("old_height", (int)vDisconnect.size() + nTargetHeight));
    result.push_back(Pair("new_height", nTargetHeight));
    result.push_back(Pair("blocks_disconnected", (int)vDisconnect.size()));
    result.push_back(Pair("new_best_hash", hashBestChain.GetHex()));

    return result;
}
