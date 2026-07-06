#ifdef ENABLE_MWEB

#include "mw/state/mw_state.h"
#include "util.h"
#include "serialize.h"
#include "hash.h"
#include "support/cleanse.h"

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>

#include <boost/filesystem.hpp>

namespace mw {

static const std::string DB_PREFIX_OUTPUT    = "o";
static const std::string DB_PREFIX_MMRINDEX  = "i";
static const std::string DB_PREFIX_KERNEL    = "k";
static const std::string DB_PREFIX_SPENT     = "p";
static const std::string DB_KEY_MMR          = "m";
static const std::string DB_KEY_META         = "s";
static const std::string DB_KEY_VERSION      = "v";
static const std::string DB_KEY_KERNEL_COUNT = "c";

static const int MWSTATE_DB_VERSION = 1;

static leveldb::DB* OpenMWStateDB(const std::string& strPath, bool fCreate = true)
{
    boost::filesystem::path dbPath(strPath);
    if (fCreate)
        boost::filesystem::create_directories(dbPath);

    leveldb::Options options;
    options.create_if_missing = fCreate;
    options.block_cache = leveldb::NewLRUCache(8 * 1048576);
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.compression = leveldb::kNoCompression;

    leveldb::DB* pdb = NULL;
    leveldb::Status status = leveldb::DB::Open(options, strPath, &pdb);

    if (!status.ok())
    {
        printf("CMWState: error opening LevelDB at %s: %s\n",
               strPath.c_str(), status.ToString().c_str());
        delete options.block_cache;
        delete options.filter_policy;
        return NULL;
    }

    return pdb;
}

template<typename T>
static std::string SerializeToString(const T& value)
{
    CDataStream ss(SER_DISK, CLIENT_VERSION);
    ss << value;
    return ss.str();
}

template<typename T>
static bool DeserializeFromString(const std::string& strData, T& value)
{
    try {
        CDataStream ss(strData.data(), strData.data() + strData.size(),
                       SER_DISK, CLIENT_VERSION);
        ss >> value;
        return true;
    }
    catch (std::exception& e) {
        printf("CMWState: deserialization error: %s\n", e.what());
        return false;
    }
}

static std::string MakeKey(const std::string& prefix, const Commitment& commit)
{
    std::string key = prefix;
    key.append(reinterpret_cast<const char*>(commit.data), COMMITMENT_SIZE);
    return key;
}

static std::string MakeKey(const std::string& prefix, uint64_t nIndex)
{
    CDataStream ss(SER_DISK, CLIENT_VERSION);
    ss << nIndex;
    std::string key = prefix;
    key.append(ss.str());
    return key;
}

CMWState::~CMWState()
{
    LOCK(cs_mwstate);

    for (size_t i = 0; i < vKernelExcesses.size(); i++)
    {
        memory_cleanse(vKernelExcesses[i].data, COMMITMENT_SIZE);
    }
    vKernelExcesses.clear();

    for (std::map<Commitment, CMWOutput>::iterator it = mapOutputs.begin();
         it != mapOutputs.end(); ++it)
    {
        memory_cleanse(it->second.senderPubKey, PUBKEY_SIZE);
        memory_cleanse(it->second.receiverPubKey, PUBKEY_SIZE);
    }
    mapOutputs.clear();

    setSpent.clear();

    mapMMRIndex.clear();

    memory_cleanse(&nMWEBSupply, sizeof(nMWEBSupply));
}

bool CMWState::AddOutput(const CMWOutput& output)
{
    LOCK(cs_mwstate);

    if (output.IsNull())
        return false;

    if (mapOutputs.count(output.commitment) > 0)
    {
        printf("CMWState::AddOutput() : duplicate commitment\n");
        return false;
    }

    mapOutputs[output.commitment] = output;

    uint256 outputHash = output.GetOutputID();
    uint64_t mmrIndex = mmr.Append(outputHash);
    mapMMRIndex[output.commitment] = mmrIndex;

    return true;
}

bool CMWState::SpendOutput(const Commitment& commitment)
{
    LOCK(cs_mwstate);

    std::map<Commitment, CMWOutput>::iterator it = mapOutputs.find(commitment);
    if (it == mapOutputs.end())
    {
        printf("CMWState::SpendOutput() : output not found\n");
        return false;
    }

    if (setSpent.count(commitment) > 0)
    {
        printf("CMWState::SpendOutput() : double spend detected\n");
        return false;
    }

    setSpent.insert(commitment);
    mapOutputs.erase(it);

    return true;
}

bool CMWState::HasOutput(const Commitment& commitment) const
{
    LOCK(cs_mwstate);
    return mapOutputs.count(commitment) > 0;
}

bool CMWState::GetOutput(const Commitment& commitment, CMWOutput& outputOut) const
{
    LOCK(cs_mwstate);

    std::map<Commitment, CMWOutput>::const_iterator it = mapOutputs.find(commitment);
    if (it == mapOutputs.end())
        return false;

    outputOut = it->second;
    return true;
}

size_t CMWState::GetOutputCount() const
{
    LOCK(cs_mwstate);
    return mapOutputs.size();
}

std::vector<Commitment> CMWState::GetAllCommitments() const
{
    LOCK(cs_mwstate);

    std::vector<Commitment> vCommitments;
    vCommitments.reserve(mapOutputs.size());

    for (std::map<Commitment, CMWOutput>::const_iterator it = mapOutputs.begin();
         it != mapOutputs.end(); ++it)
    {
        vCommitments.push_back(it->first);
    }

    return vCommitments;
}

uint256 CMWState::GetMMRRoot() const
{
    LOCK(cs_mwstate);
    return mmr.GetRoot();
}

CMMRProof CMWState::GetOutputProof(const Commitment& commitment) const
{
    LOCK(cs_mwstate);

    std::map<Commitment, uint64_t>::const_iterator it = mapMMRIndex.find(commitment);
    if (it == mapMMRIndex.end())
        return CMMRProof();

    return mmr.GetProof(it->second);
}

bool CMWState::VerifyOutputProof(const Commitment& commitment, const CMMRProof& proof) const
{
    LOCK(cs_mwstate);

    std::map<Commitment, CMWOutput>::const_iterator it = mapOutputs.find(commitment);
    if (it == mapOutputs.end())
        return false;

    uint256 outputHash = it->second.GetOutputID();
    uint256 root = mmr.GetRoot();

    return CMMR::VerifyProof(root, outputHash, proof);
}

void CMWState::AddKernelExcess(const Commitment& excess)
{
    LOCK(cs_mwstate);
    vKernelExcesses.push_back(excess);
}

uint256 CMWState::GetLatestMWBlockHash() const
{
    LOCK(cs_mwstate);
    return hashLatestMWBlock;
}

int32_t CMWState::GetHeight() const
{
    LOCK(cs_mwstate);
    return nHeight;
}

void CMWState::SetLatestBlock(const uint256& hash, int32_t height)
{
    LOCK(cs_mwstate);
    hashLatestMWBlock = hash;
    nHeight = height;
}

int64_t CMWState::GetMWEBSupply() const
{
    LOCK(cs_mwstate);
    return nMWEBSupply;
}

void CMWState::AdjustSupply(int64_t nDelta)
{
    LOCK(cs_mwstate);
    nMWEBSupply += nDelta;
}

bool CMWState::ApplyBlock(const mw::CMWBlock& mwBlock, int32_t nBlockHeight,
                          const uint256& hashBlock, int64_t nSupplyDelta)
{
    LOCK(cs_mwstate);

    if (mapBlockUndo.count(hashBlock) > 0)
    {
        printf("CMWState::ApplyBlock() : block %s already applied (undo present)\n",
               hashBlock.ToString().substr(0, 20).c_str());
        return false;
    }

    CMWBlockUndo undo;
    undo.nMMRLeavesBefore = mmr.GetSize();
    undo.nKernelsBefore   = (uint64_t)vKernelExcesses.size();
    undo.nSupplyDelta     = nSupplyDelta;
    undo.hashPrevMWBlock  = hashLatestMWBlock;
    undo.nPrevHeight      = nHeight;

    for (size_t i = 0; i < mwBlock.body.vOutputs.size(); i++)
    {
        const CMWOutput& out = mwBlock.body.vOutputs[i];
        if (AddOutput(out))
            undo.vAddedOutputs.push_back(out.commitment);
    }

    for (size_t i = 0; i < mwBlock.body.vInputs.size(); i++)
    {
        const Commitment& commit = mwBlock.body.vInputs[i].commitment;
        CMWOutput spent;
        if (GetOutput(commit, spent))
        {
            if (SpendOutput(commit))
                undo.vSpentOutputs.push_back(spent);
        }
        else
        {

            printf("CMWState::ApplyBlock() : input spends unknown commitment at height %d\n",
                   nBlockHeight);
        }
    }

    for (size_t i = 0; i < mwBlock.body.vKernels.size(); i++)
        AddKernelExcess(mwBlock.body.vKernels[i].excess);

    nMWEBSupply += nSupplyDelta;
    hashLatestMWBlock = mwBlock.GetHash();
    nHeight = nBlockHeight;

    mapBlockUndo[hashBlock] = undo;
    dqUndoOrder.push_back(hashBlock);
    while (dqUndoOrder.size() > MW_UNDO_WINDOW)
    {
        const uint256& oldest = dqUndoOrder.front();

        std::map<uint256, CMWBlockUndo>::iterator itOld = mapBlockUndo.find(oldest);
        if (itOld != mapBlockUndo.end())
        {
            for (size_t i = 0; i < itOld->second.vSpentOutputs.size(); i++)
            {
                memory_cleanse(itOld->second.vSpentOutputs[i].senderPubKey, PUBKEY_SIZE);
                memory_cleanse(itOld->second.vSpentOutputs[i].receiverPubKey, PUBKEY_SIZE);
            }
            mapBlockUndo.erase(itOld);
        }
        dqUndoOrder.pop_front();
    }

    return true;
}

bool CMWState::DisconnectBlock(const uint256& hashBlock, bool fNoMWChange)
{
    LOCK(cs_mwstate);

    std::map<uint256, CMWBlockUndo>::iterator it = mapBlockUndo.find(hashBlock);
    if (it == mapBlockUndo.end())
    {

        if (fNoMWChange)
        {

            return true;
        }

        printf("CMWState::DisconnectBlock() : MISSING undo for MWEB block %s "
               "(reorg deeper than window or undo lost) — full resync required\n",
               hashBlock.ToString().substr(0, 20).c_str());
        return false;
    }

    const CMWBlockUndo& undo = it->second;

    for (size_t i = 0; i < undo.vSpentOutputs.size(); i++)
    {
        const CMWOutput& out = undo.vSpentOutputs[i];
        setSpent.erase(out.commitment);
        mapOutputs[out.commitment] = out;
    }

    for (size_t i = 0; i < undo.vAddedOutputs.size(); i++)
    {
        mapOutputs.erase(undo.vAddedOutputs[i]);
        mapMMRIndex.erase(undo.vAddedOutputs[i]);
    }

    if (!mmr.Rewind(undo.nMMRLeavesBefore))
    {
        printf("CMWState::DisconnectBlock() : MMR rewind to %llu leaves failed "
               "(current %llu) — state inconsistent\n",
               (unsigned long long)undo.nMMRLeavesBefore,
               (unsigned long long)mmr.GetSize());
        return false;
    }

    if ((uint64_t)vKernelExcesses.size() < undo.nKernelsBefore)
    {
        printf("CMWState::DisconnectBlock() : kernel count %zu below pre-block %llu "
               "— state inconsistent\n",
               vKernelExcesses.size(), (unsigned long long)undo.nKernelsBefore);
        return false;
    }
    while ((uint64_t)vKernelExcesses.size() > undo.nKernelsBefore)
    {
        memory_cleanse(vKernelExcesses.back().data, COMMITMENT_SIZE);
        vKernelExcesses.pop_back();
    }

    nMWEBSupply -= undo.nSupplyDelta;
    hashLatestMWBlock = undo.hashPrevMWBlock;
    nHeight = undo.nPrevHeight;

    {
        CMWBlockUndo& consumed = it->second;
        for (size_t i = 0; i < consumed.vSpentOutputs.size(); i++)
        {
            memory_cleanse(consumed.vSpentOutputs[i].senderPubKey, PUBKEY_SIZE);
            memory_cleanse(consumed.vSpentOutputs[i].receiverPubKey, PUBKEY_SIZE);
        }
        mapBlockUndo.erase(it);
        for (std::deque<uint256>::iterator dit = dqUndoOrder.begin(); dit != dqUndoOrder.end(); ++dit)
        {
            if (*dit == hashBlock) { dqUndoOrder.erase(dit); break; }
        }
    }

    return true;
}

bool CMWState::HasUndo(const uint256& hashBlock) const
{
    LOCK(cs_mwstate);
    return mapBlockUndo.count(hashBlock) > 0;
}

size_t CMWState::GetUndoWindowSize() const
{
    LOCK(cs_mwstate);
    return mapBlockUndo.size();
}

uint256 CMWState::ComputeStateHash() const
{
    LOCK(cs_mwstate);

    CDataStream ss(SER_DISK, CLIENT_VERSION);

    ss << mmr.GetRoot();
    ss << (uint64_t)mmr.GetSize();

    ss << (uint64_t)mapOutputs.size();
    for (std::map<Commitment, CMWOutput>::const_iterator it = mapOutputs.begin();
         it != mapOutputs.end(); ++it)
    {
        ss << it->first;
        ss << it->second.GetOutputID();
    }

    ss << (uint64_t)setSpent.size();
    for (std::set<Commitment>::const_iterator it = setSpent.begin();
         it != setSpent.end(); ++it)
        ss << *it;

    ss << (uint64_t)vKernelExcesses.size();
    for (size_t i = 0; i < vKernelExcesses.size(); i++)
        ss << vKernelExcesses[i];

    ss << nMWEBSupply;
    ss << nHeight;
    ss << hashLatestMWBlock;

    return Hash(ss.begin(), ss.end());
}

void CMWState::Snapshot(CMWState& snapshot) const
{
    LOCK(cs_mwstate);

    snapshot.mapOutputs = mapOutputs;
    snapshot.setSpent = setSpent;
    snapshot.mmr = mmr;
    snapshot.mapMMRIndex = mapMMRIndex;
    snapshot.vKernelExcesses = vKernelExcesses;
    snapshot.hashLatestMWBlock = hashLatestMWBlock;
    snapshot.nHeight = nHeight;
    snapshot.nMWEBSupply = nMWEBSupply;
    snapshot.mapBlockUndo = mapBlockUndo;
    snapshot.dqUndoOrder = dqUndoOrder;
}

void CMWState::Restore(const CMWState& snapshot)
{
    LOCK(cs_mwstate);

    mapOutputs = snapshot.mapOutputs;
    setSpent = snapshot.setSpent;
    mmr = snapshot.mmr;
    mapMMRIndex = snapshot.mapMMRIndex;
    vKernelExcesses = snapshot.vKernelExcesses;
    hashLatestMWBlock = snapshot.hashLatestMWBlock;
    nHeight = snapshot.nHeight;
    nMWEBSupply = snapshot.nMWEBSupply;
    mapBlockUndo = snapshot.mapBlockUndo;
    dqUndoOrder = snapshot.dqUndoOrder;
}

void CMWState::Clear()
{
    LOCK(cs_mwstate);

    mapOutputs.clear();
    setSpent.clear();
    mmr.Clear();
    mapMMRIndex.clear();
    vKernelExcesses.clear();
    hashLatestMWBlock = 0;
    nHeight = 0;
    nMWEBSupply = 0;
    mapBlockUndo.clear();
    dqUndoOrder.clear();
}

bool CMWState::SaveToDB(const std::string& strPath) const
{
    LOCK(cs_mwstate);

    leveldb::DB* pdb = OpenMWStateDB(strPath, true);
    if (!pdb)
        return false;

    leveldb::WriteBatch batch;

    batch.Put(DB_KEY_VERSION, SerializeToString(MWSTATE_DB_VERSION));

    {
        CDataStream ss(SER_DISK, CLIENT_VERSION);
        ss << hashLatestMWBlock;
        ss << nHeight;
        ss << nMWEBSupply;
        batch.Put(DB_KEY_META, ss.str());
    }

    {
        CDataStream ss(SER_DISK, CLIENT_VERSION);
        ss << mmr;
        batch.Put(DB_KEY_MMR, ss.str());
    }

    for (std::map<Commitment, CMWOutput>::const_iterator it = mapOutputs.begin();
         it != mapOutputs.end(); ++it)
    {
        std::string outputKey = MakeKey(DB_PREFIX_OUTPUT, it->first);
        batch.Put(outputKey, SerializeToString(it->second));

        std::map<Commitment, uint64_t>::const_iterator idxIt = mapMMRIndex.find(it->first);
        if (idxIt != mapMMRIndex.end())
        {
            std::string idxKey = MakeKey(DB_PREFIX_MMRINDEX, it->first);
            batch.Put(idxKey, SerializeToString(idxIt->second));
        }
    }

    for (std::set<Commitment>::const_iterator it = setSpent.begin();
         it != setSpent.end(); ++it)
    {
        std::string spentKey = MakeKey(DB_PREFIX_SPENT, *it);
        batch.Put(spentKey, std::string());

        std::map<Commitment, uint64_t>::const_iterator idxIt = mapMMRIndex.find(*it);
        if (idxIt != mapMMRIndex.end())
        {
            std::string idxKey = MakeKey(DB_PREFIX_MMRINDEX, *it);
            batch.Put(idxKey, SerializeToString(idxIt->second));
        }
    }

    {
        uint64_t nKernelCount = vKernelExcesses.size();
        batch.Put(DB_KEY_KERNEL_COUNT, SerializeToString(nKernelCount));

        for (uint64_t i = 0; i < vKernelExcesses.size(); i++)
        {
            std::string kernelKey = MakeKey(DB_PREFIX_KERNEL, i);
            batch.Put(kernelKey, SerializeToString(vKernelExcesses[i]));
        }
    }

    leveldb::WriteOptions writeOpts;
    writeOpts.sync = true;
    leveldb::Status status = pdb->Write(writeOpts, &batch);

    delete pdb;

    if (!status.ok())
    {
        printf("CMWState::SaveToDB() : write failed: %s\n", status.ToString().c_str());
        return false;
    }

    printf("CMWState::SaveToDB() : saved %zu outputs, %zu spent, %zu kernels at height %d to %s\n",
           mapOutputs.size(), setSpent.size(), vKernelExcesses.size(), nHeight, strPath.c_str());
    return true;
}

bool CMWState::LoadFromDB(const std::string& strPath)
{
    LOCK(cs_mwstate);

    if (!boost::filesystem::exists(strPath))
    {
        printf("CMWState::LoadFromDB() : database not found at %s\n", strPath.c_str());
        return false;
    }

    leveldb::DB* pdb = OpenMWStateDB(strPath, false);
    if (!pdb)
        return false;

    mapOutputs.clear();
    setSpent.clear();
    mmr.Clear();
    mapMMRIndex.clear();
    vKernelExcesses.clear();
    hashLatestMWBlock = 0;
    nHeight = 0;
    nMWEBSupply = 0;

    {
        std::string strValue;
        leveldb::Status status = pdb->Get(leveldb::ReadOptions(), DB_KEY_VERSION, &strValue);
        if (status.ok())
        {
            int nVersion = 0;
            if (!DeserializeFromString(strValue, nVersion))
            {
                printf("CMWState::LoadFromDB() : failed to read DB version\n");
                delete pdb;
                return false;
            }
            if (nVersion != MWSTATE_DB_VERSION)
            {
                printf("CMWState::LoadFromDB() : DB version mismatch (got %d, expected %d)\n",
                       nVersion, MWSTATE_DB_VERSION);
                delete pdb;
                return false;
            }
        }
        else if (!status.IsNotFound())
        {
            printf("CMWState::LoadFromDB() : error reading version: %s\n",
                   status.ToString().c_str());
            delete pdb;
            return false;
        }

    }

    {
        std::string strValue;
        leveldb::Status status = pdb->Get(leveldb::ReadOptions(), DB_KEY_META, &strValue);
        if (status.ok())
        {
            try {
                CDataStream ss(strValue.data(), strValue.data() + strValue.size(),
                               SER_DISK, CLIENT_VERSION);
                ss >> hashLatestMWBlock;
                ss >> nHeight;
                ss >> nMWEBSupply;
            }
            catch (std::exception& e) {
                printf("CMWState::LoadFromDB() : failed to parse metadata: %s\n", e.what());
                delete pdb;
                return false;
            }
        }
        else if (!status.IsNotFound())
        {
            printf("CMWState::LoadFromDB() : error reading metadata: %s\n",
                   status.ToString().c_str());
            delete pdb;
            return false;
        }
    }

    {
        std::string strValue;
        leveldb::Status status = pdb->Get(leveldb::ReadOptions(), DB_KEY_MMR, &strValue);
        if (status.ok())
        {
            try {
                CDataStream ss(strValue.data(), strValue.data() + strValue.size(),
                               SER_DISK, CLIENT_VERSION);
                ss >> mmr;
            }
            catch (std::exception& e) {
                printf("CMWState::LoadFromDB() : failed to parse MMR: %s\n", e.what());
                delete pdb;
                return false;
            }
        }
        else if (!status.IsNotFound())
        {
            printf("CMWState::LoadFromDB() : error reading MMR: %s\n",
                   status.ToString().c_str());
            delete pdb;
            return false;
        }
    }

    {
        std::string strValue;
        leveldb::Status status = pdb->Get(leveldb::ReadOptions(), DB_KEY_KERNEL_COUNT, &strValue);
        if (status.ok())
        {
            uint64_t nKernelCount = 0;
            if (!DeserializeFromString(strValue, nKernelCount))
            {
                printf("CMWState::LoadFromDB() : failed to read kernel count\n");
                delete pdb;
                return false;
            }

            vKernelExcesses.reserve(nKernelCount);
            for (uint64_t i = 0; i < nKernelCount; i++)
            {
                std::string kernelKey = MakeKey(DB_PREFIX_KERNEL, i);
                std::string kernelValue;
                status = pdb->Get(leveldb::ReadOptions(), kernelKey, &kernelValue);
                if (!status.ok())
                {
                    printf("CMWState::LoadFromDB() : failed to read kernel excess %llu: %s\n",
                           (unsigned long long)i, status.ToString().c_str());
                    delete pdb;
                    return false;
                }

                Commitment excess;
                if (!DeserializeFromString(kernelValue, excess))
                {
                    printf("CMWState::LoadFromDB() : failed to parse kernel excess %llu\n",
                           (unsigned long long)i);
                    delete pdb;
                    return false;
                }

                vKernelExcesses.push_back(excess);
            }
        }
    }

    {
        leveldb::Iterator* it = pdb->NewIterator(leveldb::ReadOptions());

        for (it->SeekToFirst(); it->Valid(); it->Next())
        {
            std::string strKey = it->key().ToString();

            if (strKey.empty())
                continue;

            char cPrefix = strKey[0];

            if (cPrefix == 'o' && strKey.size() == 1 + COMMITMENT_SIZE)
            {

                Commitment commit(reinterpret_cast<const unsigned char*>(strKey.data() + 1));

                CMWOutput output;
                if (!DeserializeFromString(it->value().ToString(), output))
                {
                    printf("CMWState::LoadFromDB() : failed to parse output\n");
                    delete it;
                    delete pdb;
                    return false;
                }

                mapOutputs[commit] = output;
            }
            else if (cPrefix == 'i' && strKey.size() == 1 + COMMITMENT_SIZE)
            {

                Commitment commit(reinterpret_cast<const unsigned char*>(strKey.data() + 1));

                uint64_t nIdx = 0;
                if (!DeserializeFromString(it->value().ToString(), nIdx))
                {
                    printf("CMWState::LoadFromDB() : failed to parse MMR index\n");
                    delete it;
                    delete pdb;
                    return false;
                }

                mapMMRIndex[commit] = nIdx;
            }
            else if (cPrefix == 'p' && strKey.size() == 1 + COMMITMENT_SIZE)
            {

                Commitment commit(reinterpret_cast<const unsigned char*>(strKey.data() + 1));
                setSpent.insert(commit);
            }

        }

        if (!it->status().ok())
        {
            printf("CMWState::LoadFromDB() : iterator error: %s\n",
                   it->status().ToString().c_str());
            delete it;
            delete pdb;
            return false;
        }

        delete it;
    }

    delete pdb;

    printf("CMWState::LoadFromDB() : loaded %zu outputs, %zu spent, %zu kernels at height %d from %s\n",
           mapOutputs.size(), setSpent.size(), vKernelExcesses.size(), nHeight, strPath.c_str());
    return true;
}

}

#endif
