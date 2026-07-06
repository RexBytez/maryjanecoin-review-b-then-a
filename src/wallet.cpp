#include "txdb.h"
#include "wallet.h"
#include "walletdb.h"
#include "crypter.h"
#include "ui_interface.h"
#include "base58.h"
#include "kernel.h"
#include "coincontrol.h"
#include "dandelion.h"
#include "coinjoin.h"
#ifdef ENABLE_MWEB
#include "mw/confidential.h"
#endif
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#if (BOOST_VERSION >= 106000)
#include <boost/bind/bind.hpp>
using namespace boost::placeholders;
#endif

using namespace std;
extern unsigned int nStakeMaxAge;

unsigned int nStakeCombineAge = 14 * 24 * 60 * 60;

int64_t gcd(int64_t n,int64_t m) { return m == 0 ? n : gcd(m, n % m); }
static uint64_t CoinWeightCost(const COutput &out)
{
    int64_t nTimeWeight = min((int64_t)GetTime() - (int64_t)out.tx->nTime, (int64_t)nStakeMaxAge);
    CBigNum bnCoinDayWeight = CBigNum(out.tx->vout[out.i].nValue) * nTimeWeight / (24 * 60 * 60);
    return bnCoinDayWeight.getuint64();
}

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t1,
                    const pair<int64_t, pair<const CWalletTx*, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

CPubKey CWallet::GenerateNewKey()
{
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY);

    RandAddSeedPerfmon();
    CKey key;
    key.MakeNewKey(fCompressed);

    if (fCompressed)
        SetMinVersion(FEATURE_COMPRPUBKEY);

    CPubKey pubkey = key.GetPubKey();

    int64_t nCreationTime = GetTime();
    mapKeyMetadata[pubkey.GetID()] = CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;

    if (!AddKey(key))
        throw std::runtime_error("CWallet::GenerateNewKey() : AddKey failed");
    return key.GetPubKey();
}

bool CWallet::AddKey(const CKey& key)
{
    CPubKey pubkey = key.GetPubKey();

    if (!CCryptoKeyStore::AddKey(key))
        return false;
    if (!fFileBacked)
        return true;
    if (!IsCrypted())
        return CWalletDB(strWalletFile).WriteKey(pubkey, key.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
    return true;
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey, const vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;
    if (!fFileBacked)
        return true;
    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey, vchCryptedSecret, mapKeyMetadata[vchPubKey.GetID()]);
    }
    return false;
}

bool CWallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

bool CWallet::AddCScript(const CScript& redeemScript)
{
    if (!CCryptoKeyStore::AddCScript(redeemScript))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteCScript(Hash160(redeemScript), redeemScript);
}

bool CWallet::Unlock(const SecureString& strWalletPassphrase)
{
    if (!IsLocked())
        return false;

    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(const MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey))
                return true;
        }
    }
    return false;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked();

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;
        BOOST_FOREACH(MasterKeyMap::value_type& pMasterKey, mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strOldWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey))
            {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                printf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                CWalletDB(strWalletFile).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();
                return true;
            }
        }
    }

    return false;
}

void CWallet::SetBestChain(const CBlockLocator& loc)
{
    CWalletDB walletdb(strWalletFile);
    walletdb.WriteBestBlock(loc);
}

class CCorruptAddress
{
public:
    IMPLEMENT_SERIALIZE
    (
        if (nType & SER_DISK)
            READWRITE(nVersion);
    )
};

bool CWallet::SetMinVersion(enum WalletFeature nVersion, CWalletDB* pwalletdbIn, bool fExplicit)
{
    if (nWalletVersion >= nVersion)
        return true;

    if (fExplicit && nVersion > nWalletMaxVersion)
            nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    if (fFileBacked)
    {
        CWalletDB* pwalletdb = pwalletdbIn ? pwalletdbIn : new CWalletDB(strWalletFile);
        if (nWalletVersion >= 40000)
        {

            CCorruptAddress corruptAddress;
            pwalletdb->WriteSetting("addrIncoming", corruptAddress);
        }
        if (nWalletVersion > 40000)
            pwalletdb->WriteMinVersion(nWalletVersion);
        if (!pwalletdbIn)
            delete pwalletdb;
    }

    return true;
}

bool CWallet::SetMaxVersion(int nVersion)
{

    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial vMasterKey;
    RandAddSeedPerfmon();

    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    RAND_bytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey(nDerivationMethodIndex);

    RandAddSeedPerfmon();
    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    RAND_bytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    printf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        if (fFileBacked)
        {
            pwalletdbEncryption = new CWalletDB(strWalletFile);
            if (!pwalletdbEncryption->TxnBegin())
                return false;
            pwalletdbEncryption->WriteMasterKey(nMasterKeyMaxID, kMasterKey);
        }

        if (!EncryptKeys(vMasterKey))
        {
            if (fFileBacked)
                pwalletdbEncryption->TxnAbort();
            exit(1);
        }

        SetMinVersion(FEATURE_WALLETCRYPT, pwalletdbEncryption, true);

        if (fFileBacked)
        {
            if (!pwalletdbEncryption->TxnCommit())
                exit(1);

            delete pwalletdbEncryption;
            pwalletdbEncryption = NULL;
        }

        Lock();
        Unlock(strWalletPassphrase);
        NewKeyPool();
        Lock();

        CDB::Rewrite(strWalletFile);

    }
    NotifyStatusChanged(this);

    return true;
}

int64_t CWallet::IncOrderPosNext(CWalletDB *pwalletdb)
{
    int64_t nRet = nOrderPosNext++;
    if (pwalletdb) {
        pwalletdb->WriteOrderPosNext(nOrderPosNext);
    } else {
        CWalletDB(strWalletFile).WriteOrderPosNext(nOrderPosNext);
    }
    return nRet;
}

CWallet::TxItems CWallet::OrderedTxItems(std::list<CAccountingEntry>& acentries, std::string strAccount)
{
    CWalletDB walletdb(strWalletFile);

    TxItems txOrdered;

    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        CWalletTx* wtx = &((*it).second);
        txOrdered.insert(make_pair(wtx->nOrderPos, TxPair(wtx, (CAccountingEntry*)0)));
    }
    acentries.clear();
    walletdb.ListAccountCreditDebit(strAccount, acentries);
    BOOST_FOREACH(CAccountingEntry& entry, acentries)
    {
        txOrdered.insert(make_pair(entry.nOrderPos, TxPair((CWalletTx*)0, &entry)));
    }

    return txOrdered;
}

void CWallet::WalletUpdateSpent(const CTransaction &tx, bool fBlock)
{

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi != mapWallet.end())
            {
                CWalletTx& wtx = (*mi).second;
                if (txin.prevout.n >= wtx.vout.size())
                    printf("WalletUpdateSpent: bad wtx %s\n", wtx.GetHash().ToString().c_str());
                else if (!wtx.IsSpent(txin.prevout.n) && IsMine(wtx.vout[txin.prevout.n]))
                {
                    printf("WalletUpdateSpent found spent coin %s MARYJ %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkSpent(txin.prevout.n);
                    wtx.WriteToDisk();
                    NotifyTransactionChanged(this, txin.prevout.hash, CT_UPDATED);
                }
            }
        }

        if (fBlock)
        {
            uint256 hash = tx.GetHash();
            map<uint256, CWalletTx>::iterator mi = mapWallet.find(hash);
            CWalletTx& wtx = (*mi).second;

            BOOST_FOREACH(const CTxOut& txout, tx.vout)
            {
                if (IsMine(txout))
                {
                    wtx.MarkUnspent(&txout - &tx.vout[0]);
                    wtx.WriteToDisk();
                    NotifyTransactionChanged(this, hash, CT_UPDATED);
                }
            }
        }

    }
}

void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
            item.second.MarkDirty();
    }
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn)
{
    uint256 hash = wtxIn.GetHash();
    {
        LOCK(cs_wallet);

        pair<map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hash, wtxIn));
        CWalletTx& wtx = (*ret.first).second;
        wtx.BindWallet(this);
        bool fInsertedNew = ret.second;
        if (fInsertedNew)
        {
            wtx.nTimeReceived = GetAdjustedTime();
            wtx.nOrderPos = IncOrderPosNext();

            wtx.nTimeSmart = wtx.nTimeReceived;
            if (wtxIn.hashBlock != 0)
            {
                if (mapBlockIndex.count(wtxIn.hashBlock))
                {
                    unsigned int latestNow = wtx.nTimeReceived;
                    unsigned int latestEntry = 0;
                    {

                        int64_t latestTolerated = latestNow + 300;
                        std::list<CAccountingEntry> acentries;
                        TxItems txOrdered = OrderedTxItems(acentries);
                        for (TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
                        {
                            CWalletTx *const pwtx = (*it).second.first;
                            if (pwtx == &wtx)
                                continue;
                            CAccountingEntry *const pacentry = (*it).second.second;
                            int64_t nSmartTime;
                            if (pwtx)
                            {
                                nSmartTime = pwtx->nTimeSmart;
                                if (!nSmartTime)
                                    nSmartTime = pwtx->nTimeReceived;
                            }
                            else
                                nSmartTime = pacentry->nTime;
                            if (nSmartTime <= latestTolerated)
                            {
                                latestEntry = nSmartTime;
                                if (nSmartTime > latestNow)
                                    latestNow = nSmartTime;
                                break;
                            }
                        }
                    }

                    unsigned int& blocktime = mapBlockIndex[wtxIn.hashBlock]->nTime;
                    wtx.nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
                }
                else
                    printf("AddToWallet() : found %s in block %s not in index\n",
                           wtxIn.GetHash().ToString().substr(0,10).c_str(),
                           wtxIn.hashBlock.ToString().c_str());
            }
        }

        bool fUpdated = false;
        if (!fInsertedNew)
        {

            if (wtxIn.hashBlock != 0 && wtxIn.hashBlock != wtx.hashBlock)
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            }
            if (wtxIn.nIndex != -1 && (wtxIn.vMerkleBranch != wtx.vMerkleBranch || wtxIn.nIndex != wtx.nIndex))
            {
                wtx.vMerkleBranch = wtxIn.vMerkleBranch;
                wtx.nIndex = wtxIn.nIndex;
                fUpdated = true;
            }
            if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
            {
                wtx.fFromMe = wtxIn.fFromMe;
                fUpdated = true;
            }
            fUpdated |= wtx.UpdateSpent(wtxIn.vfSpent);
        }

        printf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString().substr(0,10).c_str(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        if (fInsertedNew || fUpdated)
            if (!wtx.WriteToDisk())
                return false;
#ifndef QT_GUI

        CScript scriptDefaultKey;
        scriptDefaultKey.SetDestination(vchDefaultKey.GetID());
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if (txout.scriptPubKey == scriptDefaultKey)
            {
                CPubKey newDefaultKey;
                if (GetKeyFromPool(newDefaultKey, false))
                {
                    SetDefaultKey(newDefaultKey);
                    SetAddressBookName(vchDefaultKey.GetID(), "");
                }
            }
        }
#endif

        WalletUpdateSpent(wtx, (wtxIn.hashBlock != 0));

        if (fInsertedNew && fAutoMixEnabled && !wtx.IsCoinBase() && !wtx.IsCoinStake())
        {
            bool fHasUnmixedCredit = false;
            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
                if (IsMine(wtx.vout[i]) && wtx.vout[i].nValue > 0 &&
                    !IsMixedDenomination(wtx.vout[i].nValue))
                {
                    fHasUnmixedCredit = true;
                    break;
                }
            }
            if (fHasUnmixedCredit)
                QueueForAutoMix(hash);
        }

        NotifyTransactionChanged(this, hash, fInsertedNew ? CT_NEW : CT_UPDATED);

        std::string strCmd = GetArg("-walletnotify", "");

        if ( !strCmd.empty())
        {
            boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
            boost::thread t(runCommand, strCmd);
        }

    }
    return true;
}

bool CWallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate, bool fFindBlock)
{
    uint256 hash = tx.GetHash();
    {
        LOCK(cs_wallet);

        if (!mapStealthAddresses.empty())
        {

            BOOST_FOREACH(const CTxOut& txout, tx.vout)
            {
                const CScript& script = txout.scriptPubKey;

                if (script.size() != 35)
                    continue;
                if (script[0] != OP_RETURN)
                    continue;
                if (script[1] != 0x21)
                    continue;

                std::vector<unsigned char> vchEphemeral(script.begin() + 2, script.end());
                CPubKey ephemeralPubKey(vchEphemeral);
                if (!ephemeralPubKey.IsValid() || !ephemeralPubKey.IsCompressed())
                    continue;

                for (std::map<std::string, CStealthAddress>::const_iterator it = mapStealthAddresses.begin();
                     it != mapStealthAddresses.end(); ++it)
                {
                    const CStealthAddress& sxAddr = it->second;

                    CKey scanSecret;
                    CKeyID scanKeyID = sxAddr.scanPubKey.GetID();
                    if (!GetKey(scanKeyID, scanSecret))
                        continue;

                    CKey destKeyPubOnly;
                    if (!DetectStealthPayment(scanSecret, ephemeralPubKey,
                                              sxAddr.spendPubKey, destKeyPubOnly))
                        continue;

                    CPubKey destPubKey = destKeyPubOnly.GetPubKey();
                    CKeyID  destKeyID  = destPubKey.GetID();

                    bool fMatchFound = false;
                    BOOST_FOREACH(const CTxOut& txout2, tx.vout)
                    {
                        CTxDestination dest;
                        if (ExtractDestination(txout2.scriptPubKey, dest))
                        {
                            CKeyID* pkeyid = boost::get<CKeyID>(&dest);
                            if (pkeyid && *pkeyid == destKeyID)
                            {
                                fMatchFound = true;
                                break;
                            }
                        }
                    }

                    if (!fMatchFound)
                        continue;

                    if (HaveKey(destKeyID))
                        continue;

                    CKey spendSecret;
                    CKeyID spendKeyID = sxAddr.spendPubKey.GetID();
                    if (!GetKey(spendKeyID, spendSecret))
                        continue;

                    uint256 sharedSecret;
                    if (!ComputeStealthSharedSecret(scanSecret, ephemeralPubKey, sharedSecret))
                        continue;

                    CKey destKey;
                    if (!DeriveStealthSpendKey(spendSecret, sharedSecret, destKey))
                        continue;

                    if (AddKey(destKey))
                    {
                        printf("AddToWalletIfInvolvingMe: auto-imported stealth key for tx %s\n",
                               hash.ToString().substr(0,10).c_str());
                    }
                }
            }
        }

        bool fExisted = mapWallet.count(hash);
        if (fExisted && !fUpdate) return false;
        if (fExisted || IsMine(tx) || IsFromMe(tx))
        {
            CWalletTx wtx(this,tx);

            if (pblock)
                wtx.SetMerkleBranch(pblock);
            return AddToWallet(wtx);
        }
        else
            WalletUpdateSpent(tx);
    }
    return false;
}

bool CWallet::EraseFromWallet(uint256 hash)
{
    if (!fFileBacked)
        return false;
    {
        LOCK(cs_wallet);
        if (mapWallet.erase(hash))
            CWalletDB(strWalletFile).EraseTx(hash);
    }
    return true;
}

bool CWallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return true;
        }
    }
    return false;
}

int64_t CWallet::GetDebit(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return prev.vout[txin.prevout.n].nValue;
        }
    }
    return 0;
}

bool CWallet::IsChange(const CTxOut& txout) const
{
    CTxDestination address;

    if (ExtractDestination(txout.scriptPubKey, address) && ::IsMine(*this, address))
    {
        LOCK(cs_wallet);
        if (!mapAddressBook.count(address))
            return true;
    }
    return false;
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

int CWalletTx::GetRequestCount() const
{

    int nRequests = -1;
    {
        LOCK(pwallet->cs_wallet);
        if (IsCoinBase() || IsCoinStake())
        {

            if (hashBlock != 0)
            {
                map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                if (mi != pwallet->mapRequestCount.end())
                    nRequests = (*mi).second;
            }
        }
        else
        {

            map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(GetHash());
            if (mi != pwallet->mapRequestCount.end())
            {
                nRequests = (*mi).second;

                if (nRequests == 0 && hashBlock != 0)
                {
                    map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                    if (mi != pwallet->mapRequestCount.end())
                        nRequests = (*mi).second;
                    else
                        nRequests = 1;
                }
            }
        }
    }
    return nRequests;
}

void CWalletTx::GetAmounts(int64_t& nGeneratedImmature, int64_t& nGeneratedMature, list<pair<CTxDestination, int64_t> >& listReceived,
                           list<pair<CTxDestination, int64_t> >& listSent, int64_t& nFee, string& strSentAccount) const
{
    nGeneratedImmature = nGeneratedMature = nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    int64_t nDebit = GetDebit();
    if (nDebit > 0)
    {
        int64_t nValueOut = GetValueOut();
        nFee = nDebit - nValueOut;
    }

    BOOST_FOREACH(const CTxOut& txout, vout)
    {

        if (txout.scriptPubKey.empty())
            continue;

        bool fIsMine;

        if (nDebit > 0)
        {

            if (pwallet->IsChange(txout))
                continue;
            fIsMine = pwallet->IsMine(txout);
        }
        else if (!(fIsMine = pwallet->IsMine(txout)))
            continue;

        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address))
        {
            printf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                   this->GetHash().ToString().c_str());
            address = CNoDestination();
        }

        if (nDebit > 0)
            listSent.push_back(make_pair(address, txout.nValue));

        if (fIsMine)
            listReceived.push_back(make_pair(address, txout.nValue));
    }

}

void CWalletTx::GetAccountAmounts(const string& strAccount, int64_t& nGeneratedImmature, int64_t& nGeneratedMature, int64_t& nReceived,
                                  int64_t& nSent, int64_t& nFee) const
{
    nGeneratedImmature = nGeneratedMature = nReceived = nSent = nFee = 0;

    int64_t allGeneratedImmature, allGeneratedMature, allFee;
    string strSentAccount;
    list<pair<CTxDestination, int64_t> > listReceived;
    list<pair<CTxDestination, int64_t> > listSent;
    GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);

    if (strAccount == strSentAccount)
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& s, listSent)
            nSent += s.second;
        nFee = allFee;
		nGeneratedImmature = allGeneratedImmature;
		nGeneratedMature = allGeneratedMature;
    }
    {
        LOCK(pwallet->cs_wallet);
        BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& r, listReceived)
        {
            if (pwallet->mapAddressBook.count(r.first))
            {
                map<CTxDestination, string>::const_iterator mi = pwallet->mapAddressBook.find(r.first);
                if (mi != pwallet->mapAddressBook.end() && (*mi).second == strAccount)
                    nReceived += r.second;
            }
            else if (strAccount.empty())
            {
                nReceived += r.second;
            }
        }
    }
}

void CWalletTx::AddSupportingTransactions(CTxDB& txdb)
{
    vtxPrev.clear();

    const int COPY_DEPTH = 3;
    if (SetMerkleBranch() < COPY_DEPTH)
    {
        vector<uint256> vWorkQueue;
        BOOST_FOREACH(const CTxIn& txin, vin)
            vWorkQueue.push_back(txin.prevout.hash);

        {
            LOCK(pwallet->cs_wallet);
            map<uint256, const CMerkleTx*> mapWalletPrev;
            set<uint256> setAlreadyDone;
            for (unsigned int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hash = vWorkQueue[i];
                if (setAlreadyDone.count(hash))
                    continue;
                setAlreadyDone.insert(hash);

                CMerkleTx tx;
                map<uint256, CWalletTx>::const_iterator mi = pwallet->mapWallet.find(hash);
                if (mi != pwallet->mapWallet.end())
                {
                    tx = (*mi).second;
                    BOOST_FOREACH(const CMerkleTx& txWalletPrev, (*mi).second.vtxPrev)
                        mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;
                }
                else if (mapWalletPrev.count(hash))
                {
                    tx = *mapWalletPrev[hash];
                }
                else if (!fClient && txdb.ReadDiskTx(hash, tx))
                {
                    ;
                }
                else
                {
                    printf("ERROR: AddSupportingTransactions() : unsupported transaction\n");
                    continue;
                }

                int nDepth = tx.SetMerkleBranch();
                vtxPrev.push_back(tx);

                if (nDepth < COPY_DEPTH)
                {
                    BOOST_FOREACH(const CTxIn& txin, tx.vin)
                        vWorkQueue.push_back(txin.prevout.hash);
                }
            }
        }
    }

    reverse(vtxPrev.begin(), vtxPrev.end());
}

bool CWalletTx::WriteToDisk()
{
    return CWalletDB(pwallet->strWalletFile).WriteTx(GetHash(), *this);
}

int CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate)
{
    int ret = 0;

    CBlockIndex* pindex = pindexStart;
    {
        LOCK(cs_wallet);
        while (pindex)
        {

            if (nTimeFirstKey && (pindex->nTime < (nTimeFirstKey - 7200))) {
                pindex = pindex->pnext;
                continue;
            }

            CBlock block;
            block.ReadFromDisk(pindex, true);
            BOOST_FOREACH(CTransaction& tx, block.vtx)
            {
                if (AddToWalletIfInvolvingMe(tx, &block, fUpdate))
                    ret++;
            }
            pindex = pindex->pnext;
        }
    }
    return ret;
}

int CWallet::ScanForWalletTransaction(const uint256& hashTx)
{
    CTransaction tx;
    tx.ReadFromDisk(COutPoint(hashTx, 0));
    if (AddToWalletIfInvolvingMe(tx, NULL, true, true))
        return 1;
    return 0;
}

void CWallet::ReacceptWalletTransactions()
{
    CTxDB txdb("r");
    bool fRepeat = true;
    while (fRepeat)
    {
        LOCK(cs_wallet);
        fRepeat = false;
        vector<CDiskTxPos> vMissingTx;
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            if ((wtx.IsCoinBase() && wtx.IsSpent(0)) || (wtx.IsCoinStake() && wtx.IsSpent(1)))
                continue;

            CTxIndex txindex;
            bool fUpdated = false;
            if (txdb.ReadTxIndex(wtx.GetHash(), txindex))
            {

                if (txindex.vSpent.size() != wtx.vout.size())
                {
                    printf("ERROR: ReacceptWalletTransactions() : txindex.vSpent.size() %" PRIszu " != wtx.vout.size() %" PRIszu "\n", txindex.vSpent.size(), wtx.vout.size());
                    continue;
                }
                for (unsigned int i = 0; i < txindex.vSpent.size(); i++)
                {
                    if (wtx.IsSpent(i))
                        continue;
                    if (!txindex.vSpent[i].IsNull() && IsMine(wtx.vout[i]))
                    {
                        wtx.MarkSpent(i);
                        fUpdated = true;
                        vMissingTx.push_back(txindex.vSpent[i]);
                    }
                }
                if (fUpdated)
                {
                    printf("ReacceptWalletTransactions found spent coin %s MARYJ %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkDirty();
                    wtx.WriteToDisk();
                }
            }
            else
            {

                if (!(wtx.IsCoinBase() || wtx.IsCoinStake()))
                    wtx.AcceptWalletTransaction(txdb);
            }
        }
        if (!vMissingTx.empty())
        {

            if (ScanForWalletTransactions(pindexGenesisBlock))
                fRepeat = true;
        }
    }
}

void CWalletTx::RelayWalletTransaction(CTxDB& txdb)
{
    BOOST_FOREACH(const CMerkleTx& tx, vtxPrev)
    {
        if (!(tx.IsCoinBase() || tx.IsCoinStake()))
        {
            uint256 hash = tx.GetHash();
            if (!txdb.ContainsTx(hash))
                RelayTransaction((CTransaction)tx, hash);
        }
    }
    if (!(IsCoinBase() || IsCoinStake()))
    {
        uint256 hash = GetHash();
        if (!txdb.ContainsTx(hash))
        {
            printf("Relaying wtx %s\n", hash.ToString().substr(0,10).c_str());

            g_dandelion.AddLocalTx(hash);
            RelayTransaction((CTransaction)*this, hash);
        }
    }
}

void CWalletTx::RelayWalletTransaction()
{
   CTxDB txdb("r");
   RelayWalletTransaction(txdb);
}

void CWallet::ResendWalletTransactions(bool fForce)
{
    if (!fForce)
    {

        static int64_t nNextTime;
        if (GetTime() < nNextTime)
            return;
        bool fFirst = (nNextTime == 0);
        nNextTime = GetTime() + GetRand(30 * 60);
        if (fFirst)
            return;

        static int64_t nLastTime;
        if (nTimeBestReceived < nLastTime)
            return;
        nLastTime = GetTime();
    }

    printf("ResendWalletTransactions()\n");
    CTxDB txdb("r");
    {
        LOCK(cs_wallet);

        multimap<unsigned int, CWalletTx*> mapSorted;
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;

            if (fForce || nTimeBestReceived - (int64_t)wtx.nTimeReceived > 5 * 60)
                mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
        }
        BOOST_FOREACH(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
        {
            CWalletTx& wtx = *item.second;
            if (wtx.CheckTransaction())
                wtx.RelayWalletTransaction(txdb);
            else
                printf("ResendWalletTransactions() : CheckTransaction failed for transaction %s\n", wtx.GetHash().ToString().c_str());
        }
    }
}

int64_t CWallet::GetBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsFinal() && pcoin->IsConfirmed())
                nTotal += pcoin->GetAvailableCredit();
        }
    }

    return nTotal;
}

int64_t CWallet::GetBalanceV1() const
{
    int64_t nTotal = 0;
    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (pcoin->IsFinal() && pcoin->IsConfirmedV1())
                nTotal += pcoin->GetAvailableCredit();
        }
    }
    return nTotal;
}

int64_t CWallet::GetUnconfirmedBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            if ((!pcoin->IsFinal() || !pcoin->IsConfirmed()) && pcoin->GetDepthInMainChain() >= 0)
                nTotal += pcoin->GetAvailableCredit();
        }
    }
    return nTotal;
}

int64_t CWallet::GetConflictedBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            if (!(pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetDepthInMainChain() < 0)
            {

                int64_t nCredit = pcoin->GetCredit();
                int64_t nDebit = pcoin->GetDebit();
                nTotal += (nCredit - nDebit);
            }
        }
    }
    return nTotal;
}

int64_t CWallet::GetImmatureBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx& pcoin = (*it).second;
            if (pcoin.IsCoinBase() && pcoin.GetBlocksToMaturity() > 0 && pcoin.IsInMainChain())
                nTotal += GetCredit(pcoin);
        }
    }
    return nTotal;
}

int64_t CWallet::GetTransparentBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || !pcoin->IsConfirmed())
                continue;

            PoolType pool = GetOutputPool(*pcoin);
            if (pool != POOL_TRANSPARENT)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->IsSpent(i))
                    continue;
                if (!IsMine(pcoin->vout[i]))
                    continue;
                if (pcoin->vout[i].nValue == 0)
                    continue;
                nTotal += pcoin->vout[i].nValue;
            }
        }
    }
    return nTotal;
}

int64_t CWallet::GetShieldedBalance() const
{
    int64_t nTotal = 0;
    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || !pcoin->IsConfirmed())
                continue;

            PoolType pool = GetOutputPool(*pcoin);
            if (pool != POOL_SHIELDED)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->IsSpent(i))
                    continue;
                if (!IsMine(pcoin->vout[i]))
                    continue;
                if (pcoin->vout[i].nValue == 0)
                    continue;
                nTotal += pcoin->vout[i].nValue;
            }
        }
    }
    return nTotal;
}

bool CWallet::SelectCoinsTransparentOnly(int64_t nTargetValue, unsigned int nSpendTime, int nMinConf,
    set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    vector<COutput> vCoins;
    AvailableCoinsMinConf(vCoins, nMinConf);

    setCoinsRet.clear();
    nValueRet = 0;

    BOOST_FOREACH(COutput output, vCoins)
    {
        const CWalletTx *pcoin = output.tx;
        int i = output.i;

        if (nValueRet >= nTargetValue)
            break;

        if (pcoin->nTime > nSpendTime)
            continue;

        PoolType pool = GetOutputPool(*pcoin);
        if (pool != POOL_TRANSPARENT)
            continue;

        int64_t n = pcoin->vout[i].nValue;

        pair<int64_t,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n >= nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            break;
        }
        else if (n < nTargetValue + CENT)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
        }
    }

    return true;
}

void CWallet::AvailableCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl) const
{
    vCoins.clear();

    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            if (!pcoin->IsFinal())
                continue;

            if (fOnlyConfirmed && !pcoin->IsConfirmed())
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            if(pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 0)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->IsSpent(i))
                    continue;
                if (!IsMine(pcoin->vout[i]))
                    continue;
                if (pcoin->vout[i].nValue < nMinimumInputValue)
                    continue;
                if (coinControl && coinControl->HasSelected() && !coinControl->IsSelected((*it).first, i))
                    continue;

                if (coinControl && coinControl->IsFrozen((*it).first, i))
                    continue;

                vCoins.push_back(COutput(pcoin, i, nDepth));
            }

        }
    }
}

void CWallet::AvailableCoinsMinConf(vector<COutput>& vCoins, int nConf) const
{
    vCoins.clear();

    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            if (!pcoin->IsFinal())
                continue;

            if(pcoin->GetDepthInMainChain() < nConf)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
                if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue >= nMinimumInputValue)
                    vCoins.push_back(COutput(pcoin, i, pcoin->GetDepthInMainChain()));
        }
    }
}

static void ApproximateBestSubset(vector<pair<int64_t, pair<const CWalletTx*,unsigned int> > >vValue, int64_t nTotalLower, int64_t nTargetValue,
                                  vector<char>& vfBest, int64_t& nBest, int iterations = 1000)
{
    vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        int64_t nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                if (nPass == 0 ? rand() % 2 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

int64_t CWallet::GetStake() const
{
    int64_t nTotal = 0;
    LOCK(cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
            nTotal += CWallet::GetCredit(*pcoin);
    }
    return nTotal;
}

int64_t CWallet::GetNewMint() const
{
    int64_t nTotal = 0;
    LOCK(cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        const CWalletTx* pcoin = &(*it).second;
        if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0 && pcoin->GetDepthInMainChain() > 0)
            nTotal += CWallet::GetCredit(*pcoin);
    }
    return nTotal;
}

bool CWallet::MultiSend()
{
	if ( IsInitialBlockDownload() || IsLocked() || nBestHeight < GetNumBlocksOfPeers() )
        return false;
    int64_t nAmount = 0;

    {
		LOCK(cs_wallet);
		std::vector<COutput> vCoins;
		AvailableCoins(vCoins);

		if (nBestHeight <= nLastMultiSendHeight)
			return false;

	     BOOST_FOREACH(const COutput& out, vCoins)
		{
			CTxDestination address;
			if(!ExtractDestination(out.tx->vout[out.i].scriptPubKey, address)) continue;
			if (out.tx->IsCoinStake() && out.tx->GetBlocksToMaturity() == 0  && out.tx->GetDepthInMainChain() >= nCoinbaseMaturity+10)
			{

				bool fDisabledAddress = false;
				if(vDisabledAddresses.size() > 0)
				{
					for(unsigned int i = 0; i < vDisabledAddresses.size(); i++)
					{
						if(vDisabledAddresses[i] == CBitcoinAddress(address).ToString())
						{
							fDisabledAddress = true;
							break;
						}
					}
				}
				if(fDisabledAddress)
					continue;

				CCoinControl* cControl = new CCoinControl();
				uint256 txhash = out.tx->GetHash();
				COutPoint outpt(txhash, out.i);
				cControl->Select(outpt);
				CWalletTx wtx;
				cControl->fReturnChange = true;
				CReserveKey keyChange(this);
				int64_t nFeeRet = 0;
				vector<pair<CScript, int64_t> > vecSend;

				for(unsigned int i = 0; i < vMultiSend.size(); i++)
				{

					nAmount = ( ( out.tx->GetCredit() - out.tx->GetDebit() ) * vMultiSend[i].second )/100;
					CBitcoinAddress strAddSend(vMultiSend[i].first);
					CScript scriptPubKey;
						scriptPubKey.SetDestination(strAddSend.Get());
					vecSend.push_back(make_pair(scriptPubKey, nAmount));
				}

				fSplitBlock = false;

				bool fCreated = CreateTransaction(vecSend, wtx, keyChange, nFeeRet, 1, cControl);
				if (!fCreated)
				{
					printf("MultiSend createtransaction failed\n");
					delete cControl;
					continue;
				}
				if(!CommitTransaction(wtx, keyChange))
				{
					printf("MultiSend transaction commit failed\n");
					delete cControl;
					continue;
				}

				fMultiSendNotify = true;
				delete cControl;

				CWalletDB walletdb(strWalletFile);
				nLastMultiSendHeight = nBestHeight;
				if(!walletdb.WriteMSettings(fMultiSend, nLastMultiSendHeight))
					printf("Failed to write MultiSend setting to DB\n");

			}
		}
    }
    return true;
}

struct LargerOrEqualThanThreshold
{
    int64_t threshold;
    LargerOrEqualThanThreshold(int64_t threshold) : threshold(threshold) {}
    bool operator()(pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > const &v) const { return v.first.first >= threshold; }
};

bool CWallet::SelectCoinsMinConfByCoinAge(int64_t nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, std::vector<COutput> vCoins, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    vector<pair<COutput, uint64_t> > mCoins;
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        mCoins.push_back(std::make_pair(out, CoinWeightCost(out)));
    }

    pair<pair<int64_t,int64_t>, pair<const CWalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first.second = std::numeric_limits<int64_t>::max();
    coinLowestLarger.second.first = NULL;
    vector<pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > > vValue;
    int64_t nTotalLower = 0;
    boost::sort(mCoins, boost::bind(&std::pair<COutput, uint64_t>::second, _1) < boost::bind(&std::pair<COutput, uint64_t>::second, _2));

    BOOST_FOREACH(const PAIRTYPE(COutput, uint64_t)& output, mCoins)
    {
        const CWalletTx *pcoin = output.first.tx;

        if (output.first.nDepth < (pcoin->IsFromMe() ? nConfMine : nConfTheirs))
            continue;

        int i = output.first.i;

        if (pcoin->nTime > nSpendTime)
            continue;

        int64_t n = pcoin->vout[i].nValue;

        pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > coin = make_pair(make_pair(n,output.second),make_pair(pcoin, i));

        if (n < nTargetValue + CENT)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        }
        else if (output.second < (uint64_t)coinLowestLarger.first.second)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first.first;
        return true;
    }

    int64_t nTotalValue = vValue[0].first.first;
    int64_t nGCD = vValue[0].first.first;
    for (unsigned int i = 1; i < vValue.size(); ++i)
    {
        nGCD = gcd(vValue[i].first.first, nGCD);
        nTotalValue += vValue[i].first.first;
    }
    nGCD = gcd(nTargetValue, nGCD);
    int64_t denom = nGCD;
    const int64_t k = 25;
    const int64_t approx = int64_t(vValue.size() * (nTotalValue - nTargetValue)) / k;
    if (approx > nGCD)
    {
        denom = approx;
    }
    if (fDebug) cerr << "nGCD " << nGCD << " denom " << denom << " k " << k << endl;

    if (nTotalValue == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
        }
        nValueRet = nTotalValue;
        return true;
    }

    size_t nBeginBundles = vValue.size();
    size_t nTotalCoinValues = vValue.size();
    size_t nBeginCoinValues = 0;
    int64_t costsum = 0;
    vector<vector<pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > >::iterator> vZeroValueBundles;
    if (denom != nGCD)
    {

        vector<pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > >::iterator itZeroValue = std::stable_partition(vValue.begin(), vValue.end(), LargerOrEqualThanThreshold(denom));
        vZeroValueBundles.push_back(itZeroValue);
        pair<int64_t, int64_t> pBundle = make_pair(0, 0);
        nBeginBundles = itZeroValue - vValue.begin();
        nTotalCoinValues = nBeginBundles;
        while (itZeroValue != vValue.end())
        {
            pBundle.first += itZeroValue->first.first;
            pBundle.second += itZeroValue->first.second;
            itZeroValue++;
            if (pBundle.first >= denom)
            {
                vZeroValueBundles.push_back(itZeroValue);
                vValue[nTotalCoinValues].first = pBundle;
                pBundle = make_pair(0, 0);
                nTotalCoinValues++;
            }
        }

        nTotalValue = 0;
        for (unsigned int i = 0; i < nTotalCoinValues; ++i)
        {
            nTotalValue += vValue[i].first.first / denom;
        }

        if (nTargetValue/denom >= nTotalValue)
        {

            for (; nBeginCoinValues < nTotalCoinValues && (nTargetValue - nValueRet)/denom >= nTotalValue; ++nBeginCoinValues)
            {
                if (nBeginCoinValues >= nBeginBundles)
                {
                    if (fDebug) cerr << "prepick bundle item " << FormatMoney(vValue[nBeginCoinValues].first.first) << " normalized " << vValue[nBeginCoinValues].first.first / denom << " cost " << vValue[nBeginCoinValues].first.second << endl;
                    const size_t nBundle = nBeginCoinValues - nBeginBundles;
                    for (vector<pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > >::iterator it = vZeroValueBundles[nBundle]; it != vZeroValueBundles[nBundle + 1]; ++it)
                    {
                        setCoinsRet.insert(it->second);
                    }
                }
                else
                {
                    if (fDebug) cerr << "prepicking " << FormatMoney(vValue[nBeginCoinValues].first.first) << " normalized " << vValue[nBeginCoinValues].first.first / denom << " cost " << vValue[nBeginCoinValues].first.second << endl;
                    setCoinsRet.insert(vValue[nBeginCoinValues].second);
                }
                nTotalValue -= vValue[nBeginCoinValues].first.first / denom;
                nValueRet += vValue[nBeginCoinValues].first.first;
                costsum += vValue[nBeginCoinValues].first.second;
            }
            if (nValueRet >= nTargetValue)
            {
                    if (fDebug) cerr << "Done without dynprog: " << "requested " << FormatMoney(nTargetValue) << "\tnormalized " << nTargetValue/denom + (nTargetValue % denom != 0 ? 1 : 0) << "\tgot " << FormatMoney(nValueRet) << "\tcost " << costsum << endl;
                    return true;
            }
        }
    }
    else
    {
        nTotalValue /= denom;
    }

    uint64_t nAppend = 1;
    if ((nTargetValue - nValueRet) % denom != 0)
    {

        nAppend--;
    }

    boost::numeric::ublas::matrix<uint64_t> M((nTotalCoinValues - nBeginCoinValues) + 1, (nTotalValue - (nTargetValue - nValueRet)/denom) + nAppend, std::numeric_limits<int64_t>::max());
    boost::numeric::ublas::matrix<unsigned int> B((nTotalCoinValues - nBeginCoinValues) + 1, (nTotalValue - (nTargetValue - nValueRet)/denom) + nAppend);
    for (unsigned int j = 0; j < M.size2(); ++j)
    {
        M(0,j) = 0;
    }
    for (unsigned int i = 1; i < M.size1(); ++i)
    {
        uint64_t nWeight = vValue[nBeginCoinValues + i - 1].first.first / denom;
        uint64_t nValue = vValue[nBeginCoinValues + i - 1].first.second;

        for (unsigned int j = 0; j < M.size2(); ++j)
        {
            B(i, j) = j;
            if (nWeight <= j)
            {
                uint64_t nStep = M(i - 1, j - nWeight) + nValue;
                if (M(i - 1, j) >= nStep)
                {
                    M(i, j) = M(i - 1, j);
                }
                else
                {
                    M(i, j) = nStep;
                    B(i, j) = j - nWeight;
                }
            }
            else
            {
                M(i, j) = M(i - 1, j);
            }
        }
    }

    int64_t nPrev = M.size2() - 1;
    for (unsigned int i = M.size1() - 1; i > 0; --i)
    {

        if (nPrev == B(i, nPrev))
        {
            const size_t nValue = nBeginCoinValues + i - 1;

            if (nValue >= nBeginBundles)
            {
                if (fDebug) cerr << "pick bundle item " << FormatMoney(vValue[nValue].first.first) << " normalized " << vValue[nValue].first.first / denom << " cost " << vValue[nValue].first.second << endl;
                const size_t nBundle = nValue - nBeginBundles;
                for (vector<pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > >::iterator it = vZeroValueBundles[nBundle]; it != vZeroValueBundles[nBundle + 1]; ++it)
                {
                    setCoinsRet.insert(it->second);
                }
            }
            else
            {
                if (fDebug) cerr << "pick " << nValue << " value " << FormatMoney(vValue[nValue].first.first) << " normalized " << vValue[nValue].first.first / denom << " cost " << vValue[nValue].first.second << endl;
                setCoinsRet.insert(vValue[nValue].second);
            }
            nValueRet += vValue[nValue].first.first;
            costsum += vValue[nValue].first.second;
        }
        nPrev = B(i, nPrev);
    }
    if (nValueRet < nTargetValue && !vZeroValueBundles.empty())
    {

        for (vector<pair<pair<int64_t,int64_t>,pair<const CWalletTx*,unsigned int> > >::iterator it = vZeroValueBundles.back(); it != vValue.end() && nValueRet < nTargetValue; ++it)
        {
            setCoinsRet.insert(it->second);
            nValueRet += it->first.first;
        }
    }
    if (fDebug) cerr << "requested " << FormatMoney(nTargetValue) << "\tnormalized " << nTargetValue/denom + (nTargetValue % denom != 0 ? 1 : 0) << "\tgot " << FormatMoney(nValueRet) << "\tcost " << costsum << endl;
    if (fDebug) cerr << "M " << M.size1() << "x" << M.size2() << "; vValue.size() = " << vValue.size() << endl;
    return true;
}

bool CWallet::SelectCoinsMinConf(int64_t nTargetValue, unsigned int nSpendTime, int nConfMine, int nConfTheirs, vector<COutput> vCoins, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    pair<int64_t, pair<const CWalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<int64_t>::max();
    coinLowestLarger.second.first = NULL;
    vector<pair<int64_t, pair<const CWalletTx*,unsigned int> > > vValue;
    int64_t nTotalLower = 0;

    boost::range::detail::random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

    BOOST_FOREACH(COutput output, vCoins)
    {
        const CWalletTx *pcoin = output.tx;

        if (output.nDepth < (pcoin->IsFromMe() ? nConfMine : nConfTheirs))
            continue;

        int i = output.i;

        if (pcoin->nTime > nSpendTime)
            continue;

        int64_t n = pcoin->vout[i].nValue;

        pair<int64_t,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n == nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            return true;
        }
        else if (n < nTargetValue + CENT)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        }
        else if (n < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }

    sort(vValue.rbegin(), vValue.rend(), CompareValueOnly());
    vector<char> vfBest;
    int64_t nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest, 1000);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + CENT)
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + CENT, vfBest, nBest, 1000);

    if (coinLowestLarger.second.first &&
        ((nBest != nTargetValue && nBest < nTargetValue + CENT) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    }
    else {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        if (fDebug && GetBoolArg("-printpriority"))
        {

            printf("SelectCoins() best subset: ");
            for (unsigned int i = 0; i < vValue.size(); i++)
                if (vfBest[i])
                    printf("%s ", FormatMoney(vValue[i].first).c_str());
            printf("total %s\n", FormatMoney(nBest).c_str());
        }
    }

    return true;
}

bool CWallet::SelectCoinsPrivacy(int64_t nTargetValue, unsigned int nSpendTime, vector<COutput> vCoins, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, const CCoinControl* coinControl) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    if (vCoins.empty())
        return false;

    vector<COutput> vEligible;
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if (out.tx->nTime > nSpendTime)
            continue;
        if (out.nDepth < 1)
            continue;
        vEligible.push_back(out);
    }

    if (vEligible.empty())
        return false;

    BOOST_FOREACH(const COutput& out, vEligible)
    {
        int64_t nVal = out.tx->vout[out.i].nValue;
        if (nVal == nTargetValue)
        {
            setCoinsRet.insert(make_pair(out.tx, (unsigned int)out.i));
            nValueRet = nVal;
            return true;
        }
    }

    if (coinControl && coinControl->fDontConsolidate)
    {

        map<CTxDestination, vector<COutput> > mapByAddr;
        BOOST_FOREACH(const COutput& out, vEligible)
        {
            CTxDestination dest;
            if (ExtractDestination(out.tx->vout[out.i].scriptPubKey, dest))
                mapByAddr[dest].push_back(out);
        }

        for (map<CTxDestination, vector<COutput> >::iterator it = mapByAddr.begin(); it != mapByAddr.end(); ++it)
        {
            int64_t nGroupTotal = 0;
            BOOST_FOREACH(const COutput& out, it->second)
                nGroupTotal += out.tx->vout[out.i].nValue;

            if (nGroupTotal >= nTargetValue)
            {

                vector<pair<int64_t, const COutput*> > vSorted;
                BOOST_FOREACH(const COutput& out, it->second)
                    vSorted.push_back(make_pair(out.tx->vout[out.i].nValue, &out));

                sort(vSorted.rbegin(), vSorted.rend());

                int64_t nAccum = 0;
                for (size_t i = 0; i < vSorted.size() && nAccum < nTargetValue; ++i)
                {
                    setCoinsRet.insert(make_pair(vSorted[i].second->tx, (unsigned int)vSorted[i].second->i));
                    nAccum += vSorted[i].first;
                }
                nValueRet = nAccum;
                return (nValueRet >= nTargetValue);
            }
        }

        return false;
    }

    map<int, vector<COutput> > mapByBucket;
    BOOST_FOREACH(const COutput& out, vEligible)
    {
        int64_t nAge = (int64_t)GetTime() - (int64_t)out.tx->nTime;
        int nBucket = CCoinControl::GetAgeBucket(nAge);
        mapByBucket[nBucket].push_back(out);
    }

    for (int nBucket = 3; nBucket >= 0; --nBucket)
    {
        if (mapByBucket.count(nBucket) == 0)
            continue;

        vector<COutput>& vBucket = mapByBucket[nBucket];
        int64_t nBucketTotal = 0;
        BOOST_FOREACH(const COutput& out, vBucket)
            nBucketTotal += out.tx->vout[out.i].nValue;

        if (nBucketTotal >= nTargetValue)
        {

            vector<pair<int64_t, const COutput*> > vSorted;
            BOOST_FOREACH(const COutput& out, vBucket)
                vSorted.push_back(make_pair(out.tx->vout[out.i].nValue, &out));

            sort(vSorted.rbegin(), vSorted.rend());

            int64_t nAccum = 0;
            for (size_t i = 0; i < vSorted.size() && nAccum < nTargetValue; ++i)
            {
                setCoinsRet.insert(make_pair(vSorted[i].second->tx, (unsigned int)vSorted[i].second->i));
                nAccum += vSorted[i].first;
            }
            nValueRet = nAccum;
            return true;
        }
    }

    vector<pair<int, const COutput*> > vByScore;
    BOOST_FOREACH(const COutput& out, vEligible)
        vByScore.push_back(make_pair(CCoinControl::GetPrivacyScore(out.nDepth), &out));

    sort(vByScore.rbegin(), vByScore.rend());

    int64_t nAccum = 0;
    for (size_t i = 0; i < vByScore.size() && nAccum < nTargetValue; ++i)
    {
        const COutput* pout = vByScore[i].second;
        setCoinsRet.insert(make_pair(pout->tx, (unsigned int)pout->i));
        nAccum += pout->tx->vout[pout->i].nValue;
    }
    nValueRet = nAccum;
    return (nValueRet >= nTargetValue);
}

bool CWallet::MintableCoins()
{
	vector<COutput> vCoins;
    AvailableCoins(vCoins, true);

	BOOST_FOREACH(const COutput& out, vCoins)
	{
		if(GetTime() - out.tx->GetTxTime() > nStakeMinAge)
			return true;
	}
	return false;
}

bool CWallet::SelectCoins(int64_t nTargetValue, unsigned int nSpendTime, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, const CCoinControl* coinControl) const
{

    if (nBestHeight >= RING_MIXING_MANDATORY_HEIGHT && (!coinControl || !coinControl->HasSelected()))
    {
        if (SelectCoinsPreferMixed(nTargetValue, nSpendTime, setCoinsRet, nValueRet, coinControl))
            return true;
    }

    vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);

    if (coinControl && coinControl->HasSelected())
    {
        BOOST_FOREACH(const COutput& out, vCoins)
        {
            nValueRet += out.tx->vout[out.i].nValue;
            setCoinsRet.insert(make_pair(out.tx, out.i));
        }
        return (nValueRet >= nTargetValue);
    }

    boost::function<bool (const CWallet*, int64_t, unsigned int, int, int, std::vector<COutput>, std::set<std::pair<const CWalletTx*,unsigned int> >&, int64_t&)> f = fMinimizeCoinAge ? &CWallet::SelectCoinsMinConfByCoinAge : &CWallet::SelectCoinsMinConf;

    return (f(this, nTargetValue, nSpendTime, 1, 10, vCoins, setCoinsRet, nValueRet) ||
            f(this, nTargetValue, nSpendTime, 1, 1, vCoins, setCoinsRet, nValueRet) ||
            f(this, nTargetValue, nSpendTime, 0, 1, vCoins, setCoinsRet, nValueRet));
}

bool CWallet::SelectCoinsForPayJoin(const vector<int64_t>& vTargetDenominations,
                                     int nMaxInputs,
                                     set<pair<const CWalletTx*, unsigned int> >& setCoinsRet,
                                     int64_t& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    vector<COutput> vCoins;
    AvailableCoins(vCoins, true);

    if (vCoins.empty())
        return false;

    vector<pair<int64_t, int> > vScored;
    for (int idx = 0; idx < (int)vCoins.size(); idx++)
    {
        const COutput& out = vCoins[idx];

        if (out.nDepth < 1)
            continue;

        CTxDestination addr;
        if (ExtractDestination(out.tx->vout[out.i].scriptPubKey, addr))
        {
            map<CTxDestination, string>::const_iterator mi = mapAddressBook.find(addr);
            if (mi != mapAddressBook.end())
            {
                string strLabel = mi->second;
                if (strLabel == "frozen" || strLabel == "private" || strLabel == "do-not-spend")
                    continue;
            }
        }

        int64_t nValue = out.tx->vout[out.i].nValue;

        int64_t nBestDiff = nValue;
        for (size_t i = 0; i < vTargetDenominations.size(); i++)
        {
            int64_t nDiff = std::abs(nValue - vTargetDenominations[i]);
            if (nDiff < nBestDiff)
                nBestDiff = nDiff;
        }

        vScored.push_back(make_pair(nBestDiff, idx));
    }

    sort(vScored.begin(), vScored.end());

    int nSelected = 0;
    for (size_t i = 0; i < vScored.size() && nSelected < nMaxInputs; i++)
    {
        const COutput& out = vCoins[vScored[i].second];
        int64_t nValue = out.tx->vout[out.i].nValue;

        setCoinsRet.insert(make_pair(out.tx, (unsigned int)out.i));
        nValueRet += nValue;
        nSelected++;
    }

    return nSelected > 0;
}

bool CWallet::SelectCoinsSimple(int64_t nTargetValue, unsigned int nSpendTime, int nMinConf, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const
{
    vector<COutput> vCoins;
    AvailableCoinsMinConf(vCoins, nMinConf);

    setCoinsRet.clear();
    nValueRet = 0;

    BOOST_FOREACH(COutput output, vCoins)
    {
        const CWalletTx *pcoin = output.tx;
        int i = output.i;

        if (nValueRet >= nTargetValue)
            break;

        if (pcoin->nTime > nSpendTime)
            continue;

        int64_t n = pcoin->vout[i].nValue;

        pair<int64_t,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin, i));

        if (n >= nTargetValue)
        {

            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            break;
        }
        else if (n < nTargetValue + CENT)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
        }
    }

    return true;
}

bool CWallet::GetStakeWeightFromValue(const int64_t nTime, const int64_t nValue, uint64_t& nWeight)
{

	int64_t nTimeWeight = GetWeight2(nTime, (int64_t)GetTime());
	if (nTimeWeight < 0 )
		nTimeWeight=0;

	CBigNum bnCoinDayWeight = CBigNum(nValue) * nTimeWeight / COIN / (24 * 60 * 60);
	nWeight = bnCoinDayWeight.getuint64();
	return true;
}

bool CWallet::CreateTransaction(const vector<pair<CScript, int64_t> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, int nSplitBlock, const CCoinControl* coinControl)
{
    int64_t nValue = 0;
    BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
    {
        if (nValue < 0)
            return false;
        nValue += s.second;
    }
    if (vecSend.empty() || nValue < 0)
        return false;

    wtxNew.BindWallet(this);

    {
        LOCK2(cs_main, cs_wallet);

        CTxDB txdb("r");
        {
            nFeeRet = nTransactionFee;
			if(fSplitBlock)
				nFeeRet = COIN * 1;
            while (true)
            {
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;

                int64_t nTotalValue = nValue + nFeeRet;
                double dPriority = 0;
				if( nSplitBlock < 1 )
					nSplitBlock = 1;

                bool fRingMixingActive = (nBestHeight >= RING_MIXING_MANDATORY_HEIGHT);

                if (fRingMixingActive && !fSplitBlock)
                {

                    BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
                    {
                        int64_t nSendValue = s.second;

                        int64_t nBestDenom = 0;
                        for (int d = 0; d < COINJOIN_NUM_DENOMINATIONS; d++)
                        {
                            if (nSendValue >= COINJOIN_DENOMINATIONS[d] * RING_MIXING_MIN_EQUAL_OUTPUTS)
                            {
                                nBestDenom = COINJOIN_DENOMINATIONS[d];
                                break;
                            }
                        }

                        if (nBestDenom > 0)
                        {

                            int nOutputCount = (int)(nSendValue / nBestDenom);
                            if (nOutputCount > COINJOIN_MAX_OUTPUTS)
                                nOutputCount = COINJOIN_MAX_OUTPUTS;

                            int64_t nDenomTotal = nBestDenom * nOutputCount;
                            int64_t nRemainder = nSendValue - nDenomTotal;

                            for (int i = 0; i < nOutputCount; i++)
                                wtxNew.vout.push_back(CTxOut(nBestDenom, s.first));

                            if (nRemainder > 0)
                                wtxNew.vout.push_back(CTxOut(nRemainder, s.first));
                        }
                        else
                        {

                            int64_t nEqualPart = nSendValue / RING_MIXING_MIN_EQUAL_OUTPUTS;
                            int64_t nLastPart = nSendValue - (nEqualPart * (RING_MIXING_MIN_EQUAL_OUTPUTS - 1));

                            for (int i = 0; i < RING_MIXING_MIN_EQUAL_OUTPUTS - 1; i++)
                                wtxNew.vout.push_back(CTxOut(nEqualPart, s.first));
                            wtxNew.vout.push_back(CTxOut(nLastPart, s.first));
                        }
                    }
                }
                else if (!fSplitBlock)
				{
				BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
					wtxNew.vout.push_back(CTxOut(s.second, s.first));
				}
				else
                BOOST_FOREACH (const PAIRTYPE(CScript, int64_t)& s, vecSend)
				{
                    for(int nCount = 0; nCount < nSplitBlock; nCount++)
					{
						if(nCount == nSplitBlock -1)
						{
							uint64_t nRemainder = s.second % nSplitBlock;
							wtxNew.vout.push_back(CTxOut((s.second / nSplitBlock) + nRemainder, s.first));
						}
						else
							wtxNew.vout.push_back(CTxOut(s.second / nSplitBlock, s.first));
					}
				}

                set<pair<const CWalletTx*,unsigned int> > setCoins;
                int64_t nValueIn = 0;
                if (!SelectCoins(nTotalValue, wtxNew.nTime, setCoins, nValueIn, coinControl))
                    return false;
				CTxDestination utxoAddress;
                BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
                {
                    int64_t nCredit = pcoin.first->vout[pcoin.second].nValue;
                    dPriority += (double)nCredit * pcoin.first->GetDepthInMainChain();

					ExtractDestination(pcoin.first->vout[pcoin.second].scriptPubKey, utxoAddress);
                }

                int64_t nChange = nValueIn - nValue - nFeeRet;

                if (nFeeRet < MIN_TX_FEE && nChange > 0 && nChange < CENT)
                {
                    int64_t nMoveToFee = min(nChange, MIN_TX_FEE - nFeeRet);
                    nChange -= nMoveToFee;
                    nFeeRet += nMoveToFee;
                }

                if (nChange > 0)
                {

                    CScript scriptChange;
					if (coinControl && coinControl->fReturnChange == true)
						scriptChange.SetDestination(utxoAddress);

                    else if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                        scriptChange.SetDestination(coinControl->destChange);

                    else
                    {

                        CPubKey vchPubKey = reservekey.GetReservedKey();

                        scriptChange.SetDestination(vchPubKey.GetID());
                    }

                    vector<CTxOut>::iterator position = wtxNew.vout.begin()+GetRandInt(wtxNew.vout.size());
                    wtxNew.vout.insert(position, CTxOut(nChange, scriptChange));
                }
                else
                    reservekey.ReturnKey();

                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    wtxNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second));

                int nIn = 0;
                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    if (!SignSignature(*this, *coin.first, wtxNew, nIn++))
                        return false;

                unsigned int nBytes = ::GetSerializeSize(*(CTransaction*)&wtxNew, SER_NETWORK, PROTOCOL_VERSION);
                if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
                    return false;
                dPriority /= nBytes;

                int64_t nPayFee = nTransactionFee * (1 + (int64_t)nBytes / 1000);
                int64_t nMinFee = wtxNew.GetMinFee(1, GMF_SEND, nBytes);

                if (nFeeRet < max(nPayFee, nMinFee))
                {
                    nFeeRet = max(nPayFee, nMinFee);
                    continue;
                }

                wtxNew.AddSupportingTransactions(txdb);
                wtxNew.fTimeReceivedIsTxTime = true;

                break;
            }
        }
    }
    return true;
}

bool CWallet::CreateTransaction(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64_t& nFeeRet, const CCoinControl* coinControl)
{
    vector< pair<CScript, int64_t> > vecSend;
    vecSend.push_back(make_pair(scriptPubKey, nValue));
    return CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet, 1, coinControl);
}

#ifdef ENABLE_MWEB

bool CWallet::CreateMWEBPegInTransaction(int64_t nAmount,
                                          CWalletTx& wtxNew,
                                          CReserveKey& reservekey,
                                          int64_t& nFeeRet,
                                          mw::CMWOwnedOutput& mwOutputOut)
{
    extern mw::CMWWallet g_mwWallet;

    if (nAmount <= 0)
        return false;

    if (!g_mwWallet.HasKeys())
    {
        if (!g_mwWallet.GenerateKeys())
            return false;
    }

    mw::CPegInResult pegResult = mw::CreatePegIn(this, nAmount);
    if (!pegResult.fSuccess)
    {
        printf("CreateMWEBPegInTransaction() : peg-in failed: %s\n",
               pegResult.strError.c_str());
        return false;
    }

    CScript scriptMarker = mw::GetPegInMarkerScript(pegResult.mwOutput.commitment);

    std::vector<std::pair<CScript, int64_t> > vecSend;
    vecSend.push_back(std::make_pair(scriptMarker, 0));

    nFeeRet = nAmount + MIN_TX_FEE;

    if (!CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet, 1, NULL))
    {
        printf("CreateMWEBPegInTransaction() : failed to create transparent TX\n");
        return false;
    }

    wtxNew.nVersion = CT_TX_VERSION;

    mw::CMWTransactionBody body;
    body.vOutputs.push_back(pegResult.mwOutput);
    body.vKernels.push_back(pegResult.kernel);

    wtxNew.mwTx = mw::CMWTransaction(body, mw::BlindingFactor());

    mwOutputOut.output = pegResult.mwOutput;
    mwOutputOut.blindingFactor = pegResult.outputBlind;
    mwOutputOut.nValue = nAmount;
    mwOutputOut.nBlockHeight = 0;
    mwOutputOut.fSpent = false;

    printf("CreateMWEBPegInTransaction() : created peg-in TX for %" PRId64 " MARYJ\n", nAmount);
    return true;
}
#endif

bool CWallet::GetStakeWeight(const CKeyStore& keystore, uint64_t& nMinWeight, uint64_t& nMaxWeight, uint64_t& nWeight)
{

    int64_t nBalance = GetBalance();

    int64_t nReserveBalance = 0;
    if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        return error("CreateCoinStake : invalid reserve balance amount");
    if (nBalance <= nReserveBalance)
        return false;

    vector<const CWalletTx*> vwtxPrev;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    int64_t nValueIn = 0;

    if (!SelectCoinsSimple(nBalance - nReserveBalance, GetTime(), nCoinbaseMaturity + 5, setCoins, nValueIn))
        return false;

    if (setCoins.empty())
        return false;

    CTxDB txdb("r");
    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        CTxIndex txindex;
        {
            LOCK2(cs_main, cs_wallet);
            if (!txdb.ReadTxIndex(pcoin.first->GetHash(), txindex))
                continue;
        }

        int64_t nTimeWeight = GetWeight((int64_t)pcoin.first->nTime, (int64_t)GetTime());
        CBigNum bnCoinDayWeight = CBigNum(pcoin.first->vout[pcoin.second].nValue) * nTimeWeight / COIN / (24 * 60 * 60);

        if (nTimeWeight > 0)
        {
            nWeight += bnCoinDayWeight.getuint64();
        }

        if (nTimeWeight > 0 && nTimeWeight < nStakeMaxAge)
        {
            nMinWeight += bnCoinDayWeight.getuint64();
        }

        if (nTimeWeight == nStakeMaxAge)
        {
            nMaxWeight += bnCoinDayWeight.getuint64();
        }
    }

    return true;
}

bool CWallet::GetStakeWeight2(const CKeyStore& keystore, uint64_t& nMinWeight, uint64_t& nMaxWeight, uint64_t& nWeight, uint64_t& nHoursToMaturity, uint64_t& nAmount)
{

    int64_t nBalance = GetBalance();

    int64_t nReserveBalance = 0;
    if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        return error("CreateCoinStake : invalid reserve balance amount");
    if (nBalance <= nReserveBalance)
        return false;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    vector<const CWalletTx*> vwtxPrev;
    int64_t nValueIn = 0;

    if (!SelectCoins(nBalance - nReserveBalance, GetTime(), setCoins, nValueIn))
        return false;

    if (setCoins.empty())
        return false;

	uint64_t nPrevAge = 0;
	uint64_t nStakeAge = nStakeMinAge;

    CTxDB txdb("r");
    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        CTxIndex txindex;
       {
            LOCK2(cs_main, cs_wallet);
            if (!txdb.ReadTxIndex(pcoin.first->GetHash(), txindex))
                continue;
        }

		uint64_t nCurrentAge = (int64_t)GetTime() - (int64_t)pcoin.first->nTime;
		if (nCurrentAge > nPrevAge)
		{
			nPrevAge = nCurrentAge;
			nHoursToMaturity = ((nStakeAge - nPrevAge) / 60 / 60) + 1;
		}

        int64_t nTimeWeight = GetWeight2((int64_t)pcoin.first->nTime, (int64_t)GetTime());
        CBigNum bnCoinDayWeight = CBigNum(pcoin.first->vout[pcoin.second].nValue) * nTimeWeight / COIN / (24 * 60 * 60);

		if (nCurrentAge < nStakeAge)
			bnCoinDayWeight = 0;

        if (nTimeWeight > 0)
        {
            nWeight += bnCoinDayWeight.getuint64();
			nAmount += (uint64_t)pcoin.first->vout[pcoin.second].nValue / COIN;
        }

        if (nTimeWeight > 0 && nTimeWeight < nStakeMaxAge)
        {
            nMinWeight += bnCoinDayWeight.getuint64();
        }

        if (nTimeWeight == nStakeMaxAge)
        {
            nMaxWeight += bnCoinDayWeight.getuint64();
        }
    }

    return true;
}

bool CWallet::CreateCoinStake(const CKeyStore& keystore, unsigned int nBits, int64_t nSearchInterval, int64_t nFees, CTransaction& txNew, CKey& key)
{
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    txNew.vin.clear();
    txNew.vout.clear();

    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));

    int64_t nBalance;
    bool fTwoPoolActive = (nBestHeight >= TWO_POOL_ACTIVATION_HEIGHT);
    if (fTwoPoolActive)
        nBalance = GetTransparentBalance();
    else
        nBalance = GetBalance();

    int64_t nReserveBalance = 0;
    if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        return error("CreateCoinStake : invalid reserve balance amount");

    if (nBalance <= nReserveBalance)
        return false;

    vector<const CWalletTx*> vwtxPrev;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    int64_t nValueIn = 0;

    if (fTwoPoolActive)
    {
        if (!SelectCoinsTransparentOnly(nBalance - nReserveBalance, txNew.nTime, nCoinbaseMaturity + 5, setCoins, nValueIn))
            return false;
    }
    else
    {
        if (!SelectCoinsSimple(nBalance - nReserveBalance, txNew.nTime, nCoinbaseMaturity + 5, setCoins, nValueIn))
            return false;
    }

    if (setCoins.empty())
        return false;

    printf("CreateCoinStake: trying %d coins, nBits=%u, txNew.nTime=%u, now=%ld\n", (int)setCoins.size(), nBits, txNew.nTime, (long)GetTime());

    int64_t nCredit = 0;
    CScript scriptPubKeyKernel;
    CTxDB txdb("r");
    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {
        CTxIndex txindex;
        {
            LOCK2(cs_main, cs_wallet);
            if (!txdb.ReadTxIndex(pcoin.first->GetHash(), txindex))
            {
                static int nReadFail = 0;
                if (nReadFail++ % 100 == 0) printf("CreateCoinStake: ReadTxIndex FAILED for %d coins\n", nReadFail);
                continue;
            }
        }

        CBlock block;
        {
            LOCK2(cs_main, cs_wallet);
            if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
            {
                printf("CreateCoinStake: ReadFromDisk FAILED\n");
                continue;
            }
        }

        if (block.GetBlockTime() + nStakeMinAge > txNew.nTime)
        {
            static int nSkipCount = 0;
            if (nSkipCount++ % 100 == 0)
                printf("CreateCoinStake: SKIPPING coin - blockTime=%u + minAge=%u = %u > txTime=%u (too young by %d sec)\n",
                    (unsigned int)block.GetBlockTime(), nStakeMinAge,
                    (unsigned int)(block.GetBlockTime() + nStakeMinAge), txNew.nTime,
                    (int)(block.GetBlockTime() + nStakeMinAge - txNew.nTime));
            continue;
        }

        printf("CreateCoinStake: checking coin value=%" PRId64 " blockTime=%u\n", pcoin.first->vout[pcoin.second].nValue, (unsigned int)block.GetBlockTime());
        bool fKernelFound = false;

		uint256 hashProofOfStake = 0;
		COutPoint prevoutStake = COutPoint(pcoin.first->GetHash(), pcoin.second);
		unsigned int txNewTime = txNew.nTime;
		if (CheckStakeKernelHash(nBits, block, txindex.pos.nTxPos - txindex.pos.nBlockPos, *pcoin.first, prevoutStake, txNewTime, nHashDrift, false, hashProofOfStake))
		{

			if (fDebug && GetBoolArg("-printcoinstake"))
				printf("CreateCoinStake : kernel found\n");
			vector<valtype> vSolutions;
			txnouttype whichType;
			CScript scriptPubKeyOut;
			scriptPubKeyKernel = pcoin.first->vout[pcoin.second].scriptPubKey;
			if (!Solver(scriptPubKeyKernel, whichType, vSolutions))
			{
				if (fDebug && GetBoolArg("-printcoinstake"))
					printf("CreateCoinStake : failed to parse kernel\n");
				break;
			}
			if (fDebug && GetBoolArg("-printcoinstake"))
				printf("CreateCoinStake : parsed kernel type=%d\n", whichType);
			if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH)
			{
				if (fDebug && GetBoolArg("-printcoinstake"))
				printf("CreateCoinStake : no support for kernel type=%d\n", whichType);
				break;
			}
			if (whichType == TX_PUBKEYHASH)
			{

				if (!keystore.GetKey(uint160(vSolutions[0]), key))
				{
					if (fDebug && GetBoolArg("-printcoinstake"))
						printf("CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
					break;
				}
				scriptPubKeyOut << key.GetPubKey() << OP_CHECKSIG;
			}
			if (whichType == TX_PUBKEY)
			{
				valtype& vchPubKey = vSolutions[0];
				if (!keystore.GetKey(Hash160(vchPubKey), key))
				{
					if (fDebug && GetBoolArg("-printcoinstake"))
						printf("CreateCoinStake : failed to get key for kernel type=%d\n", whichType);
					break;
				}
                if (key.GetPubKey() != vchPubKey)
                {
                    if (fDebug && GetBoolArg("-printcoinstake"))
						printf("CreateCoinStake : invalid key for kernel type=%d\n", whichType);
					break;
				}
				scriptPubKeyOut = scriptPubKeyKernel;
			}

			txNew.nTime = txNewTime;
			txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
			nCredit += pcoin.first->vout[pcoin.second].nValue;
			vwtxPrev.push_back(pcoin.first);
			txNew.vout.push_back(CTxOut(0, scriptPubKeyOut));

			uint64_t nTotalSize = pcoin.first->vout[pcoin.second].nValue * (1+((txNew.nTime - block.GetBlockTime()) / (60*60*24)) * (MAX_MINT_PROOF_OF_STAKE / COIN / 365));
			if (nTotalSize / 2 > nStakeSplitThreshold * COIN)
				txNew.vout.push_back(CTxOut(0, scriptPubKeyOut));
			if (fDebug && GetBoolArg("-printcoinstake"))
				printf("CreateCoinStake : added kernel type=%d\n", whichType);
			fKernelFound = true;
			break;
		}
		if (fKernelFound || fShutdown)
			break;
    }

    if (nCredit == 0 || nCredit > nBalance - nReserveBalance)
        return false;

    BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
    {

        if (txNew.vout.size() == 2 && ((pcoin.first->vout[pcoin.second].scriptPubKey == scriptPubKeyKernel || pcoin.first->vout[pcoin.second].scriptPubKey == txNew.vout[1].scriptPubKey))
            && pcoin.first->GetHash() != txNew.vin[0].prevout.hash)
        {
            int64_t nTimeWeight = GetWeight((int64_t)pcoin.first->nTime, (int64_t)txNew.nTime);

            if (txNew.vin.size() >= 100)
                break;

            if (nCredit >= nCombineThreshold)
                break;

            if (nCredit + pcoin.first->vout[pcoin.second].nValue > nBalance - nReserveBalance)
                break;

            if (pcoin.first->vout[pcoin.second].nValue >= nCombineThreshold)
                continue;

            if (nTimeWeight < nStakeCombineAge)
                continue;

            txNew.vin.push_back(CTxIn(pcoin.first->GetHash(), pcoin.second));
            nCredit += pcoin.first->vout[pcoin.second].nValue;
            vwtxPrev.push_back(pcoin.first);
        }
    }

	uint256 prevHash = 0;
	if(pindexBest->pprev)
		prevHash = pindexBest->GetBlockHash();

    int64_t nReward = 0;
    {
        uint64_t nCoinAge;
        CTxDB txdb("r");
        if (!txNew.GetCoinAge(txdb, nCoinAge))
            return error("CreateCoinStake : failed to calculate coin age");

        nReward = GetProofOfStakeReward(nCoinAge, nBits, txNew.nTime, nFees, nCredit, prevHash, nBestHeight + 1);
        if (nReward < 0)
            return false;

        nCredit += nReward;
    }

    unsigned int nOriginalOutputCount = txNew.vout.size();
    int64_t nTotalCharity = 0;
    int nCharityOutputCount = 0;
    if (fStakeForCharity && !vStakeForCharity.empty())
    {

        for (unsigned int i = 0; i < vStakeForCharity.size() && i < 5; i++)
        {
            int64_t nCharityAmount = (nReward * vStakeForCharity[i].nPercent) / 100;
            if (nCharityAmount > 0 && nCharityAmount >= MIN_RELAY_TX_FEE)
            {
                CBitcoinAddress charityAddress(vStakeForCharity[i].strAddress);
                if (charityAddress.IsValid())
                {
                    CScript scriptPubKeyCharity;
                    scriptPubKeyCharity.SetDestination(charityAddress.Get());

                    txNew.vout.insert(txNew.vout.begin() + 1 + nCharityOutputCount, CTxOut(nCharityAmount, scriptPubKeyCharity));
                    nTotalCharity += nCharityAmount;
                    nCharityOutputCount++;

                    if (fDebug && GetBoolArg("-printcoinstake"))
                        printf("CreateCoinStake: Adding charity output: %s to %s (%d%%)\n",
                            FormatMoney(nCharityAmount).c_str(),
                            vStakeForCharity[i].strAddress.c_str(),
                            vStakeForCharity[i].nPercent);
                }
            }
        }

        if (nTotalCharity > 0)
        {
            if (nTotalCharity > nReward)
            {
                if (fDebug && GetBoolArg("-printcoinstake"))
                    printf("CreateCoinStake: WARNING - nTotalCharity (%s) > nReward (%s), capping to reward\n",
                        FormatMoney(nTotalCharity).c_str(), FormatMoney(nReward).c_str());
                nTotalCharity = nReward;
            }
            nCredit -= nTotalCharity;
        }
    }

    if (nCredit <= 0)
    {
        if (fDebug && GetBoolArg("-printcoinstake"))
            printf("CreateCoinStake: ERROR - nCredit is %s after charity deduction (nReward=%s, nTotalCharity=%s)\n",
                FormatMoney(nCredit).c_str(), FormatMoney(nReward).c_str(), FormatMoney(nTotalCharity).c_str());
        return false;
    }

    int nFirstStakerOutput = 1 + nCharityOutputCount;

    if (nFirstStakerOutput >= (int)txNew.vout.size())
    {
        if (fDebug && GetBoolArg("-printcoinstake"))
            printf("CreateCoinStake: ERROR - nFirstStakerOutput (%d) >= vout.size() (%d)\n",
                nFirstStakerOutput, (int)txNew.vout.size());
        return false;
    }

    if (nOriginalOutputCount == 3)
    {

        if (nFirstStakerOutput + 1 >= (int)txNew.vout.size())
        {
            if (fDebug && GetBoolArg("-printcoinstake"))
                printf("CreateCoinStake: ERROR - split stake but nFirstStakerOutput+1 (%d) >= vout.size() (%d)\n",
                    nFirstStakerOutput + 1, (int)txNew.vout.size());
            return false;
        }
        txNew.vout[nFirstStakerOutput].nValue = (nCredit / 2 / CENT) * CENT;
        txNew.vout[nFirstStakerOutput + 1].nValue = nCredit - txNew.vout[nFirstStakerOutput].nValue;

        if (fDebug && GetBoolArg("-printcoinstake"))
            printf("CreateCoinStake: Split stake - output[%d]=%s, output[%d]=%s (nCredit=%s)\n",
                nFirstStakerOutput, FormatMoney(txNew.vout[nFirstStakerOutput].nValue).c_str(),
                nFirstStakerOutput + 1, FormatMoney(txNew.vout[nFirstStakerOutput + 1].nValue).c_str(),
                FormatMoney(nCredit).c_str());
    }
    else
    {
        txNew.vout[nFirstStakerOutput].nValue = nCredit;

        if (fDebug && GetBoolArg("-printcoinstake"))
            printf("CreateCoinStake: Single stake - output[%d]=%s (nCredit=%s, nReward=%s, nTotalCharity=%s)\n",
                nFirstStakerOutput, FormatMoney(txNew.vout[nFirstStakerOutput].nValue).c_str(),
                FormatMoney(nCredit).c_str(), FormatMoney(nReward).c_str(), FormatMoney(nTotalCharity).c_str());
    }

    int nIn = 0;
    BOOST_FOREACH(const CWalletTx* pcoin, vwtxPrev)
    {
        if (!SignSignature(*this, *pcoin, txNew, nIn++))
            return error("CreateCoinStake : failed to sign coinstake");
    }

    unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
    if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
        return error("CreateCoinStake : exceeded coinstake size limit");

    return true;
}

bool CWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey)
{
    {
        LOCK2(cs_main, cs_wallet);
        printf("CommitTransaction:\n%s", wtxNew.ToString().c_str());
        {

            CWalletDB* pwalletdb = fFileBacked ? new CWalletDB(strWalletFile,"r") : NULL;

            reservekey.KeepKey();

            AddToWallet(wtxNew);

            set<CWalletTx*> setCoins;
            BOOST_FOREACH(const CTxIn& txin, wtxNew.vin)
            {
                CWalletTx &coin = mapWallet[txin.prevout.hash];
                coin.BindWallet(this);
                coin.MarkSpent(txin.prevout.n);
                coin.WriteToDisk();
                NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
            }

            if (fFileBacked)
                delete pwalletdb;
        }

        mapRequestCount[wtxNew.GetHash()] = 0;

        if (!wtxNew.AcceptToMemoryPool())
        {

            printf("CommitTransaction() : Error: Transaction not valid\n");
            return false;
        }
        wtxNew.RelayWalletTransaction();
    }
    return true;
}

string CWallet::SendMoney(CScript scriptPubKey, int64_t nValue, CWalletTx& wtxNew, bool fAskFee, bool fAllowStakeForCharity)
{
    CReserveKey reservekey(this);
    int64_t nFeeRequired;

    if (IsLocked())
    {
        string strError = _("Error: Wallet locked, unable to create transaction  ");
        printf("SendMoney() : %s", strError.c_str());
        return strError;
    }
    if (fWalletUnlockMintOnly && !fAllowStakeForCharity )
    {
        string strError = _("Error: Wallet unlocked for staking only, unable to create transaction.");
        printf("SendMoney() : %s", strError.c_str());
        return strError;
    }
    if (!CreateTransaction(scriptPubKey, nValue, wtxNew, reservekey, nFeeRequired))
    {
        string strError;
        if (nValue + nFeeRequired > GetBalance())
            strError = strprintf(_("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds  "), FormatMoney(nFeeRequired).c_str());
        else
            strError = _("Error: Transaction creation failed  ");
        printf("SendMoney() : %s", strError.c_str());
        return strError;
    }

    if (fAskFee && !uiInterface.ThreadSafeAskFee(nFeeRequired, _("Sending...")))
        return "ABORTED";

    if (!CommitTransaction(wtxNew, reservekey))
        return _("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    return "";
}

string CWallet::SendMoneyToDestination(const CTxDestination& address, int64_t nValue, CWalletTx& wtxNew, bool fAskFee, bool fAllowStakeForCharity)
{

    if (nValue <= 0)
        return _("Invalid amount");
    if (nValue + nTransactionFee > GetBalance())
        return _("Insufficient funds");

    CScript scriptPubKey;
    scriptPubKey.SetDestination(address);

    return SendMoney(scriptPubKey, nValue, wtxNew, fAskFee, fAllowStakeForCharity);
}

DBErrors CWallet::LoadWallet(bool& fFirstRunRet)
{
    if (!fFileBacked)
        return DB_LOAD_OK;
    fFirstRunRet = false;
    DBErrors nLoadWalletRet = CWalletDB(strWalletFile,"cr+").LoadWallet(this);
    if (nLoadWalletRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            setKeyPool.clear();

        }
    }

    if (nLoadWalletRet != DB_LOAD_OK)
        return nLoadWalletRet;
    fFirstRunRet = !vchDefaultKey.IsValid();

    NewThread(ThreadFlushWalletDB, &strWalletFile);
    return DB_LOAD_OK;
}

bool CWallet::SetAddressBookName(const CTxDestination& address, const string& strName)
{
    std::map<CTxDestination, std::string>::iterator mi = mapAddressBook.find(address);
    mapAddressBook[address] = strName;
    NotifyAddressBookChanged(this, address, strName, ::IsMine(*this, address), (mi == mapAddressBook.end()) ? CT_NEW : CT_UPDATED);
    if (!fFileBacked)
        return false;
    return CWalletDB(strWalletFile).WriteName(CBitcoinAddress(address).ToString(), strName);
}

bool CWallet::DelAddressBookName(const CTxDestination& address)
{
    mapAddressBook.erase(address);
    NotifyAddressBookChanged(this, address, "", ::IsMine(*this, address), CT_DELETED);
    if (!fFileBacked)
        return false;
    return CWalletDB(strWalletFile).EraseName(CBitcoinAddress(address).ToString());
}

void CWallet::PrintWallet(const CBlock& block)
{
    {
        LOCK(cs_wallet);
        if (block.IsProofOfWork() && mapWallet.count(block.vtx[0].GetHash()))
        {
            CWalletTx& wtx = mapWallet[block.vtx[0].GetHash()];
            printf("    mine:  %d  %d  %" PRId64 "", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
        }
        if (block.IsProofOfStake() && mapWallet.count(block.vtx[1].GetHash()))
        {
            CWalletTx& wtx = mapWallet[block.vtx[1].GetHash()];
            printf("    stake: %d  %d  %" PRId64 "", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
         }

    }
    printf("\n");
}

bool CWallet::GetTransaction(const uint256 &hashTx, CWalletTx& wtx)
{
    {
        LOCK(cs_wallet);
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
        {
            wtx = (*mi).second;
            return true;
        }
    }
    return false;
}

bool CWallet::SetDefaultKey(const CPubKey &vchPubKey)
{
    if (fFileBacked)
    {
        if (!CWalletDB(strWalletFile).WriteDefaultKey(vchPubKey))
            return false;
    }
    vchDefaultKey = vchPubKey;
    return true;
}

bool GetWalletFile(CWallet* pwallet, string &strWalletFileOut)
{
    if (!pwallet->fFileBacked)
        return false;
    strWalletFileOut = pwallet->strWalletFile;
    return true;
}

bool CWallet::NewKeyPool()
{
    {
        LOCK(cs_wallet);
        CWalletDB walletdb(strWalletFile);
        BOOST_FOREACH(int64_t nIndex, setKeyPool)
            walletdb.ErasePool(nIndex);
        setKeyPool.clear();

        if (IsLocked())
            return false;

        int64_t nKeys = max(GetArg("-keypool", 100), (int64_t)0);
        for (int i = 0; i < nKeys; i++)
        {
            int64_t nIndex = i+1;
            walletdb.WritePool(nIndex, CKeyPool(GenerateNewKey()));
            setKeyPool.insert(nIndex);
        }
        printf("CWallet::NewKeyPool wrote %" PRId64 " new keys\n", nKeys);
    }
    return true;
}

bool CWallet::TopUpKeyPool(unsigned int nSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked())
            return false;

        CWalletDB walletdb(strWalletFile);

        unsigned int nTargetSize;
        if (nSize > 0)
            nTargetSize = nSize;
        else
            nTargetSize = max(GetArg("-keypool", 100), (int64_t)0);

        while (setKeyPool.size() < (nTargetSize + 1))
        {
            int64_t nEnd = 1;
            if (!setKeyPool.empty())
                nEnd = *(--setKeyPool.end()) + 1;
            if (!walletdb.WritePool(nEnd, CKeyPool(GenerateNewKey())))
                throw runtime_error("TopUpKeyPool() : writing generated key failed");
            setKeyPool.insert(nEnd);
            printf("keypool added key %" PRId64 ", size=%" PRIszu "\n", nEnd, setKeyPool.size());
        }
    }
    return true;
}

void CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_wallet);

        if (!IsLocked())
            TopUpKeyPool();

        if(setKeyPool.empty())
            return;

        CWalletDB walletdb(strWalletFile);

        nIndex = *(setKeyPool.begin());
        setKeyPool.erase(setKeyPool.begin());
        if (!walletdb.ReadPool(nIndex, keypool))
            throw runtime_error("ReserveKeyFromKeyPool() : read failed");
        if (!HaveKey(keypool.vchPubKey.GetID()))
            throw runtime_error("ReserveKeyFromKeyPool() : unknown key in key pool");
        assert(keypool.vchPubKey.IsValid());
        if (fDebug && GetBoolArg("-printkeypool"))
            printf("keypool reserve %" PRId64 "\n", nIndex);
    }
}

int64_t CWallet::AddReserveKey(const CKeyPool& keypool)
{
    {
        LOCK2(cs_main, cs_wallet);
        CWalletDB walletdb(strWalletFile);

        int64_t nIndex = 1 + *(--setKeyPool.end());
        if (!walletdb.WritePool(nIndex, keypool))
            throw runtime_error("AddReserveKey() : writing added key failed");
        setKeyPool.insert(nIndex);
        return nIndex;
    }
    return -1;
}

void CWallet::KeepKey(int64_t nIndex)
{

    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.ErasePool(nIndex);
    }
    if(fDebug)
        printf("keypool keep %" PRId64 "\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex)
{

    {
        LOCK(cs_wallet);
        setKeyPool.insert(nIndex);
    }
    if(fDebug)
        printf("keypool return %" PRId64 "\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey& result, bool fAllowReuse)
{
    int64_t nIndex = 0;
    CKeyPool keypool;
    {
        LOCK(cs_wallet);
        ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex == -1)
        {
            if (fAllowReuse && vchDefaultKey.IsValid())
            {
                result = vchDefaultKey;
                return true;
            }
            if (IsLocked()) return false;
            result = GenerateNewKey();
            return true;
        }
        KeepKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

int64_t CWallet::GetOldestKeyPoolTime()
{
    int64_t nIndex = 0;
    CKeyPool keypool;
    ReserveKeyFromKeyPool(nIndex, keypool);
    if (nIndex == -1)
        return GetTime();
    ReturnKey(nIndex);
    return keypool.nTime;
}

std::map<CTxDestination, int64_t> CWallet::GetAddressBalances()
{
    map<CTxDestination, int64_t> balances;

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
        {
            CWalletTx *pcoin = &walletEntry.second;

            if (!pcoin->IsFinal() || !pcoin->IsConfirmed())
                continue;

            if ((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < (pcoin->IsFromMe() ? 0 : 1))
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(pcoin->vout[i]))
                    continue;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, addr))
                    continue;

                int64_t n = pcoin->IsSpent(i) ? 0 : pcoin->vout[i].nValue;

                if (!balances.count(addr))
                    balances[addr] = 0;
                balances[addr] += n;
            }
        }
    }

    return balances;
}

set< set<CTxDestination> > CWallet::GetAddressGroupings()
{
    set< set<CTxDestination> > groupings;
    set<CTxDestination> grouping;

    BOOST_FOREACH(PAIRTYPE(uint256, CWalletTx) walletEntry, mapWallet)
    {
        CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->vin.size() > 0 && IsMine(pcoin->vin[0]))
        {

            BOOST_FOREACH(CTxIn txin, pcoin->vin)
            {
                CTxDestination address;
                if(!ExtractDestination(mapWallet[txin.prevout.hash].vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
            }

            BOOST_FOREACH(CTxOut txout, pcoin->vout)
                if (IsChange(txout))
                {
                    CWalletTx tx = mapWallet[pcoin->vin[0].prevout.hash];
                    CTxDestination txoutAddr;
                    if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                        continue;
                    grouping.insert(txoutAddr);
                }
            groupings.insert(grouping);
            grouping.clear();
        }

        for (unsigned int i = 0; i < pcoin->vout.size(); i++)
            if (IsMine(pcoin->vout[i]))
            {
                CTxDestination address;
                if(!ExtractDestination(pcoin->vout[i].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
    }

    set< set<CTxDestination>* > uniqueGroupings;
    map< CTxDestination, set<CTxDestination>* > setmap;
    BOOST_FOREACH(set<CTxDestination> grouping, groupings)
    {

        set< set<CTxDestination>* > hits;
        map< CTxDestination, set<CTxDestination>* >::iterator it;
        BOOST_FOREACH(CTxDestination address, grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);

        set<CTxDestination>* merged = new set<CTxDestination>(grouping);
        BOOST_FOREACH(set<CTxDestination>* hit, hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);

        BOOST_FOREACH(CTxDestination element, *merged)
            setmap[element] = merged;
    }

    set< set<CTxDestination> > ret;
    BOOST_FOREACH(set<CTxDestination>* uniqueGrouping, uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }

    return ret;
}

void CWallet::FixSpentCoins(int& nMismatchFound, int64_t& nBalanceInQuestion, int& nOrphansFound, bool fCheckOnly)
{
    nMismatchFound = 0;
    nBalanceInQuestion = 0;
    nOrphansFound = 0;

    LOCK(cs_wallet);
    vector<CWalletTx*> vCoins;
    vCoins.reserve(mapWallet.size());
    for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        vCoins.push_back(&(*it).second);

    CTxDB txdb("r");
    BOOST_FOREACH(CWalletTx* pcoin, vCoins)
    {
        uint256 hash = pcoin->GetHash();

        CTxIndex txindex;
        bool fTxInBlockchain = txdb.ReadTxIndex(hash, txindex);

        if (!(pcoin->IsCoinBase() || pcoin->IsCoinStake()) && !fTxInBlockchain && pcoin->GetDepthInMainChain() < 0 && pcoin->GetDebit() == 0)
        {

            bool fHasReceived = false;
            int64_t nReceivedAmount = 0;
            BOOST_FOREACH(const CTxOut& txout, pcoin->vout)
            {
                if (IsMine(txout))
                {
                    fHasReceived = true;
                    nReceivedAmount += txout.nValue;
                }
            }

            if (fHasReceived)
            {
                nMismatchFound++;
                nBalanceInQuestion += nReceivedAmount;
                if (!fCheckOnly)
                {
                    printf("FixSpentCoins removing conflicted receive tx %s (amount: %s MARYJ)\n",
                           hash.ToString().c_str(), FormatMoney(nReceivedAmount).c_str());
                    EraseFromWallet(hash);
                    NotifyTransactionChanged(this, hash, CT_DELETED);
                }
                else
                {
                    printf("FixSpentCoins found conflicted receive tx %s (amount: %s MARYJ), repair not attempted\n",
                           hash.ToString().c_str(), FormatMoney(nReceivedAmount).c_str());
                }
                continue;
            }
        }

        if (!(pcoin->IsCoinBase() || pcoin->IsCoinStake()) && !fTxInBlockchain && pcoin->GetDepthInMainChain() < 0 && pcoin->GetDebit() > 0)
        {

            bool fRestoredInputs = false;
            BOOST_FOREACH(const CTxIn& txin, pcoin->vin)
            {
                map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
                if (mi != mapWallet.end())
                {
                    CWalletTx& prev = (*mi).second;
                    if (txin.prevout.n < prev.vout.size() && IsMine(prev.vout[txin.prevout.n]))
                    {

                        CTxIndex prevTxIndex;
                        bool fPrevInBlockchain = txdb.ReadTxIndex(prev.GetHash(), prevTxIndex);
                        if (fPrevInBlockchain && (prevTxIndex.vSpent.size() <= txin.prevout.n || prevTxIndex.vSpent[txin.prevout.n].IsNull()))
                        {

                            if (prev.IsSpent(txin.prevout.n))
                            {
                                prev.MarkUnspent(txin.prevout.n);
                                prev.WriteToDisk();
                                fRestoredInputs = true;
                                NotifyTransactionChanged(this, prev.GetHash(), CT_UPDATED);
                            }
                        }
                    }
                }
            }

            int64_t nCredit = pcoin->GetCredit();
            int64_t nDebit = pcoin->GetDebit();
            int64_t nNet = nCredit - nDebit;
            nMismatchFound++;

            nBalanceInQuestion += (nNet < 0 ? -nNet : nNet);
            if (!fCheckOnly)
            {
                printf("FixSpentCoins removing conflicted outgoing tx %s (net: %s MARYJ, debit: %s MARYJ, credit: %s MARYJ)\n",
                       hash.ToString().c_str(), FormatMoney(nNet).c_str(), FormatMoney(nDebit).c_str(), FormatMoney(nCredit).c_str());
                if (fRestoredInputs)
                    printf("FixSpentCoins restored %d input(s) for conflicted tx %s\n", (int)pcoin->vin.size(), hash.ToString().c_str());
                EraseFromWallet(hash);
                NotifyTransactionChanged(this, hash, CT_DELETED);
            }
            else
            {
                printf("FixSpentCoins found conflicted outgoing tx %s (net: %s MARYJ), repair not attempted\n",
                       hash.ToString().c_str(), FormatMoney(nNet).c_str());
            }
            continue;
        }

        if (!fTxInBlockchain && !(pcoin->IsCoinBase() || pcoin->IsCoinStake()))
            continue;
        for (unsigned int n=0; n < pcoin->vout.size(); n++)
        {
            bool fUpdated = false;
            if (IsMine(pcoin->vout[n]) && pcoin->IsSpent(n) && (txindex.vSpent.size() <= n || txindex.vSpent[n].IsNull()))
            {
                printf("FixSpentCoins found lost coin %s MARYJ %s[%d], %s\n",
                    FormatMoney(pcoin->vout[n].nValue).c_str(), hash.ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                nMismatchFound++;
                nBalanceInQuestion += pcoin->vout[n].nValue;
                if (!fCheckOnly)
                {
                    fUpdated = true;
                    pcoin->MarkUnspent(n);
                    pcoin->WriteToDisk();
                }
            }
            else if (IsMine(pcoin->vout[n]) && !pcoin->IsSpent(n) && (txindex.vSpent.size() > n && !txindex.vSpent[n].IsNull()))
            {
                printf("FixSpentCoins found spent coin %s MARYJ %s[%d], %s\n",
                    FormatMoney(pcoin->vout[n].nValue).c_str(), hash.ToString().c_str(), n, fCheckOnly? "repair not attempted" : "repairing");
                nMismatchFound++;
                nBalanceInQuestion += pcoin->vout[n].nValue;
                if (!fCheckOnly)
                {
                    fUpdated = true;
                    pcoin->MarkSpent(n);
                    pcoin->WriteToDisk();
                }
            }
            if (fUpdated)
                NotifyTransactionChanged(this, hash, CT_UPDATED);
        }

        if((pcoin->IsCoinBase() || pcoin->IsCoinStake()) && pcoin->GetDepthInMainChain() <= 0)
        {
           nOrphansFound++;
           if (!fCheckOnly)
           {
             EraseFromWallet(hash);
             NotifyTransactionChanged(this, hash, CT_DELETED);
           }
           printf("FixSpentCoins %s orphaned generation tx %s\n", fCheckOnly ? "found" : "removed", hash.ToString().c_str());
        }
     }
}

void CWallet::DisableTransaction(const CTransaction &tx)
{
    if (!tx.IsCoinStake() || !IsFromMe(tx))
        return;

    LOCK(cs_wallet);
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size() && IsMine(prev.vout[txin.prevout.n]))
            {
                prev.MarkUnspent(txin.prevout.n);
                prev.WriteToDisk();
            }
        }
    }
}

CPubKey CReserveKey::GetReservedKey()
{
    if (nIndex == -1)
    {
        CKeyPool keypool;
        pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
        if (nIndex != -1)
            vchPubKey = keypool.vchPubKey;
        else
        {
            printf("CReserveKey::GetReservedKey(): Warning: Using default key instead of a new key, top up your keypool!");
            vchPubKey = pwallet->vchDefaultKey;
        }
    }
    assert(vchPubKey.IsValid());
    return vchPubKey;
}

void CReserveKey::KeepKey()
{
    if (nIndex != -1)
        pwallet->KeepKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey()
{
    if (nIndex != -1)
        pwallet->ReturnKey(nIndex);
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CWallet::GetAllReserveKeys(set<CKeyID>& setAddress) const
{
    setAddress.clear();

    CWalletDB walletdb(strWalletFile);

    LOCK2(cs_main, cs_wallet);
    BOOST_FOREACH(const int64_t& id, setKeyPool)
    {
        CKeyPool keypool;
        if (!walletdb.ReadPool(id, keypool))
            throw runtime_error("GetAllReserveKeyHashes() : read failed");
        assert(keypool.vchPubKey.IsValid());
        CKeyID keyID = keypool.vchPubKey.GetID();
        if (!HaveKey(keyID))
            throw runtime_error("GetAllReserveKeyHashes() : unknown key in key pool");
        setAddress.insert(keyID);
    }
}

void CWallet::UpdatedTransaction(const uint256 &hashTx)
{
    {
        LOCK(cs_wallet);

        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
            NotifyTransactionChanged(this, hashTx, CT_UPDATED);
    }
}

void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const {
    mapKeyBirth.clear();

    for (std::map<CKeyID, CKeyMetadata>::const_iterator it = mapKeyMetadata.begin(); it != mapKeyMetadata.end(); it++)
        if (it->second.nCreateTime)
            mapKeyBirth[it->first] = it->second.nCreateTime;

    CBlockIndex *pindexMax = FindBlockByHeight(std::max(0, nBestHeight - 144));
    std::map<CKeyID, CBlockIndex*> mapKeyFirstBlock;
    std::set<CKeyID> setKeys;
    GetKeys(setKeys);
    BOOST_FOREACH(const CKeyID &keyid, setKeys) {
        if (mapKeyBirth.count(keyid) == 0)
            mapKeyFirstBlock[keyid] = pindexMax;
    }
    setKeys.clear();

    if (mapKeyFirstBlock.empty())
        return;

    std::vector<CKeyID> vAffected;
    for (std::map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); it++) {

        const CWalletTx &wtx = (*it).second;
        std::map<uint256, CBlockIndex*>::const_iterator blit = mapBlockIndex.find(wtx.hashBlock);
        if (blit != mapBlockIndex.end() && blit->second->IsInMainChain()) {

            int nHeight = blit->second->nHeight;
            BOOST_FOREACH(const CTxOut &txout, wtx.vout) {

                ::ExtractAffectedKeys(*this, txout.scriptPubKey, vAffected);
                BOOST_FOREACH(const CKeyID &keyid, vAffected) {

                    std::map<CKeyID, CBlockIndex*>::iterator rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && nHeight < rit->second->nHeight)
                        rit->second = blit->second;
                }
                vAffected.clear();
            }
        }
    }

    for (std::map<CKeyID, CBlockIndex*>::const_iterator it = mapKeyFirstBlock.begin(); it != mapKeyFirstBlock.end(); it++)
        mapKeyBirth[it->first] = it->second->nTime - 7200;
}

bool CWallet::AddStealthAddress(const CStealthAddress& sxAddr)
{
    LOCK(cs_wallet);

    std::string strEncoded = sxAddr.Encoded();
    if (strEncoded.empty())
        return false;

    mapStealthAddresses[strEncoded] = sxAddr;

    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        if (!walletdb.WriteStealthAddress(sxAddr))
            return false;
    }
    return true;
}

void CWallet::GetStealthAddresses(std::vector<CStealthAddress>& vStealthOut) const
{
    LOCK(cs_wallet);
    vStealthOut.clear();
    for (std::map<std::string, CStealthAddress>::const_iterator it = mapStealthAddresses.begin();
         it != mapStealthAddresses.end(); ++it)
    {
        vStealthOut.push_back(it->second);
    }
}

bool CWallet::IsStealthMandatory()
{
    return GetBoolArg("-stealthmandatory", true);
}

int CWallet::ScanBlockForStealthPayments(const CBlock& block)
{

    std::vector<CStealthAddress> vStealth;
    GetStealthAddresses(vStealth);

    if (vStealth.empty())
        return 0;

    struct StealthEntry
    {
        CKey      scanSecret;
        CPubKey   spendPubKey;
        CKey      spendSecret;
        bool      hasSpendSecret;
    };
    std::vector<StealthEntry> vEntries;

    {
        LOCK(cs_wallet);
        BOOST_FOREACH(const CStealthAddress& sxAddr, vStealth)
        {
            StealthEntry e;
            CKeyID scanKeyID  = sxAddr.scanPubKey.GetID();
            CKeyID spendKeyID = sxAddr.spendPubKey.GetID();

            if (!GetKey(scanKeyID, e.scanSecret))
                continue;

            e.spendPubKey = sxAddr.spendPubKey;
            e.hasSpendSecret = GetKey(spendKeyID, e.spendSecret);
            vEntries.push_back(e);
        }
    }

    if (vEntries.empty())
        return 0;

    int nFound = 0;

    BOOST_FOREACH(const CTransaction& tx, block.vtx)
    {

        BOOST_FOREACH(const CTxOut& txout, tx.vout)
        {
            const CScript& script = txout.scriptPubKey;

            if (script.size() != 35)
                continue;
            if (script[0] != OP_RETURN)
                continue;
            if (script[1] != 0x21)
                continue;

            std::vector<unsigned char> vchEphemeral(script.begin() + 2, script.end());
            CPubKey ephemeralPubKey(vchEphemeral);
            if (!ephemeralPubKey.IsValid() || !ephemeralPubKey.IsCompressed())
                continue;

            BOOST_FOREACH(StealthEntry& e, vEntries)
            {

                CKey destKeyPubOnly;
                if (!DetectStealthPayment(e.scanSecret, ephemeralPubKey,
                                          e.spendPubKey, destKeyPubOnly))
                    continue;

                CPubKey destPubKey = destKeyPubOnly.GetPubKey();
                CKeyID  destKeyID  = destPubKey.GetID();

                bool fMatchFound = false;
                BOOST_FOREACH(const CTxOut& txout2, tx.vout)
                {
                    CTxDestination dest;
                    if (ExtractDestination(txout2.scriptPubKey, dest))
                    {
                        CKeyID* pkeyid = boost::get<CKeyID>(&dest);
                        if (pkeyid && *pkeyid == destKeyID)
                        {
                            fMatchFound = true;
                            break;
                        }
                    }
                }

                if (!fMatchFound)
                    continue;

                {
                    LOCK(cs_wallet);
                    if (HaveKey(destKeyID))
                        continue;
                }

                if (!e.hasSpendSecret)
                    continue;

                uint256 sharedSecret;
                if (!ComputeStealthSharedSecret(e.scanSecret, ephemeralPubKey, sharedSecret))
                    continue;

                CKey destKey;
                if (!DeriveStealthSpendKey(e.spendSecret, sharedSecret, destKey))
                    continue;

                if (AddKey(destKey))
                {
                    nFound++;
                    printf("ScanBlockForStealthPayments: found stealth payment in tx %s\n",
                           tx.GetHash().ToString().substr(0,10).c_str());
                }
            }
        }
    }

    return nFound;
}

bool CWallet::AddPaymentChannel(const std::string& strKey, const CPaymentChannel& channel)
{
    LOCK(cs_wallet);
    mapPaymentChannels[strKey] = channel;
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        if (!walletdb.WritePaymentChannel(strKey, channel))
            return false;
    }
    return true;
}

static const int64_t AUTOMIX_MIN_VALUE = 200000000LL;

static const int AUTOMIX_INTERVAL = 60;

static const int AUTOMIX_MAX_PER_ROUND = 3;

bool CWallet::IsMixedDenomination(int64_t nValue)
{
    for (int i = 0; i < COINJOIN_NUM_DENOMINATIONS; i++)
    {
        if (nValue == COINJOIN_DENOMINATIONS[i])
            return true;
    }
    return false;
}

void CWallet::QueueForAutoMix(const uint256& txHash)
{
    LOCK(cs_automix);
    setAutoMixQueue.insert(txHash);
    printf("AutoMix: queued tx %s for background mixing\n",
           txHash.ToString().substr(0,10).c_str());
}

int CWallet::GetMixedUTXOCount() const
{
    int nCount = 0;
    LOCK(cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin();
         it != mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = it->second;
        if (!wtx.IsFinal() || !wtx.IsConfirmed())
            continue;
        if (wtx.IsCoinBase() && wtx.GetBlocksToMaturity() > 0)
            continue;
        if (wtx.IsCoinStake() && wtx.GetBlocksToMaturity() > 0)
            continue;
        for (unsigned int i = 0; i < wtx.vout.size(); i++)
        {
            if (wtx.IsSpent(i))
                continue;
            if (!IsMine(wtx.vout[i]))
                continue;
            if (IsMixedDenomination(wtx.vout[i].nValue))
                nCount++;
        }
    }
    return nCount;
}

int CWallet::GetUnmixedUTXOCount() const
{
    int nCount = 0;
    LOCK(cs_wallet);
    for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin();
         it != mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = it->second;
        if (!wtx.IsFinal() || !wtx.IsConfirmed())
            continue;
        if (wtx.IsCoinBase() && wtx.GetBlocksToMaturity() > 0)
            continue;
        if (wtx.IsCoinStake() && wtx.GetBlocksToMaturity() > 0)
            continue;
        for (unsigned int i = 0; i < wtx.vout.size(); i++)
        {
            if (wtx.IsSpent(i))
                continue;
            if (!IsMine(wtx.vout[i]))
                continue;
            if (wtx.vout[i].nValue >= AUTOMIX_MIN_VALUE &&
                !IsMixedDenomination(wtx.vout[i].nValue))
                nCount++;
        }
    }
    return nCount;
}

bool CWallet::DoAutoMixRound()
{
    if (IsLocked())
        return false;

    if (fWalletUnlockMintOnly)
        return false;

    vector<pair<int64_t, uint256> > vUnmixed;

    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin();
             it != mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = it->second;
            if (!wtx.IsFinal() || !wtx.IsConfirmed())
                continue;
            if (wtx.IsCoinBase() && wtx.GetBlocksToMaturity() > 0)
                continue;
            if (wtx.IsCoinStake() && wtx.GetBlocksToMaturity() > 0)
                continue;

            if (wtx.GetDepthInMainChain() < 1)
                continue;

            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
                if (wtx.IsSpent(i))
                    continue;
                if (!IsMine(wtx.vout[i]))
                    continue;
                int64_t nValue = wtx.vout[i].nValue;
                if (nValue >= AUTOMIX_MIN_VALUE && !IsMixedDenomination(nValue))
                {
                    vUnmixed.push_back(make_pair(nValue, it->first));
                }
            }
        }
    }

    if (vUnmixed.empty())
        return false;

    sort(vUnmixed.begin(), vUnmixed.end(), greater<pair<int64_t, uint256> >());

    int nMixed = 0;
    CCoinJoinMixer mixer(this);

    for (unsigned int idx = 0; idx < vUnmixed.size() && nMixed < AUTOMIX_MAX_PER_ROUND; idx++)
    {
        int64_t nValue = vUnmixed[idx].first;

        int64_t nBestDenom = mixer.FindBestDenomination(nValue);
        if (nBestDenom == 0)
            continue;

        int nOutputs = (int)(nValue / nBestDenom);
        if (nOutputs > COINJOIN_MAX_OUTPUTS)
            nOutputs = COINJOIN_MAX_OUTPUTS;
        if (nOutputs < COINJOIN_MIN_OUTPUTS)
            continue;

        int64_t nMixAmount = nBestDenom * nOutputs;

        printf("AutoMix: mixing %s MARYJ into %d outputs of %s each\n",
               FormatMoney(nMixAmount).c_str(), nOutputs,
               FormatMoney(nBestDenom).c_str());

        CCoinJoinResult result = mixer.MixAmount(nMixAmount);
        if (result.success)
        {
            nMixed++;
            {
                LOCK(cs_automix);
                nAutoMixRounds++;
            }
            printf("AutoMix: successfully mixed tx %s (%d inputs -> %d outputs of %s, fee %s)\n",
                   result.txHash.ToString().substr(0,10).c_str(),
                   result.numInputs, result.numOutputs,
                   FormatMoney(result.denomination).c_str(),
                   FormatMoney(result.feePaid).c_str());
        }
        else
        {
            printf("AutoMix: mix failed: %s\n", result.error.c_str());
        }
    }

    {
        LOCK(cs_automix);
        setAutoMixQueue.clear();
    }

    return nMixed > 0;
}

void CWallet::ThreadAutoMix(void* parg)
{
    CWallet* pwallet = (CWallet*)parg;
    printf("AutoMix: background thread started\n");

    MilliSleep(30000);

    while (!fShutdown)
    {

        for (int i = 0; i < AUTOMIX_INTERVAL && !fShutdown; i++)
            MilliSleep(1000);

        if (fShutdown)
            break;

        if (!pwallet->fAutoMixEnabled)
            continue;

        if (pwallet->IsLocked())
            continue;

        if (pwallet->fWalletUnlockMintOnly)
            continue;

        bool fHasUnmixed = false;
        {
            LOCK(pwallet->cs_automix);
            fHasUnmixed = !pwallet->setAutoMixQueue.empty();
        }

        if (!fHasUnmixed)
            fHasUnmixed = (pwallet->GetUnmixedUTXOCount() > 0);

        if (!fHasUnmixed)
            continue;

        pwallet->nLastAutoMixTime = GetTime();
        pwallet->DoAutoMixRound();
    }

    printf("AutoMix: background thread stopped\n");
}

bool CWallet::SelectCoinsPreferMixed(int64_t nTargetValue, unsigned int nSpendTime,
                                      set<pair<const CWalletTx*, unsigned int> >& setCoinsRet,
                                      int64_t& nValueRet,
                                      const CCoinControl* coinControl) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);

    if (vCoins.empty())
        return false;

    vector<COutput> vMixed;
    vector<COutput> vUnmixed;

    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if (IsMixedDenomination(out.tx->vout[out.i].nValue))
            vMixed.push_back(out);
        else
            vUnmixed.push_back(out);
    }

    int64_t nMixedTotal = 0;
    set<pair<const CWalletTx*, unsigned int> > setMixedCoins;

    sort(vMixed.begin(), vMixed.end(),
         [](const COutput& a, const COutput& b) {
             return a.tx->vout[a.i].nValue > b.tx->vout[b.i].nValue;
         });

    for (unsigned int idx = 0; idx < vMixed.size(); idx++)
    {
        const COutput& out = vMixed[idx];
        int64_t nValue = out.tx->vout[out.i].nValue;
        setMixedCoins.insert(make_pair(out.tx, out.i));
        nMixedTotal += nValue;
        if (nMixedTotal >= nTargetValue)
        {
            setCoinsRet = setMixedCoins;
            nValueRet = nMixedTotal;
            return true;
        }
    }

    return false;
}
