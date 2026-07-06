#ifndef BITCOIN_WALLETDB_H
#define BITCOIN_WALLETDB_H

#include "db.h"
#include "base58.h"

class CKeyPool;
class CAccount;
class CAccountingEntry;

struct StakeForCharityEntry
{
    std::string strAddress;
    int nPercent;

    StakeForCharityEntry()
    {
        strAddress = "";
        nPercent = 0;
    }

    StakeForCharityEntry(const std::string& address, int percent)
    {
        strAddress = address;
        nPercent = percent;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(strAddress);
        READWRITE(nPercent);
    )
};

enum DBErrors
{
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_NONCRITICAL_ERROR,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

class CKeyMetadata
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime;

    CKeyMetadata()
    {
        SetNull();
    }
    CKeyMetadata(int64_t nCreateTime_)
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = nCreateTime_;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
    )

    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
    }
};

class CWalletDB : public CDB
{
public:
    CWalletDB(std::string strFilename, const char* pszMode="r+") : CDB(strFilename.c_str(), pszMode)
    {
    }
private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);
public:
    bool WriteName(const std::string& strAddress, const std::string& strName);

    bool EraseName(const std::string& strAddress);

    bool WriteTx(uint256 hash, const CWalletTx& wtx)
    {
        nWalletDBUpdated++;
        return Write(std::make_pair(std::string("tx"), hash), wtx);
    }

    bool EraseTx(uint256 hash)
    {
        nWalletDBUpdated++;
        return Erase(std::make_pair(std::string("tx"), hash));
    }

    bool WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata &keyMeta)
    {
        nWalletDBUpdated++;

        if(!Write(std::make_pair(std::string("keymeta"), vchPubKey), keyMeta))
            return false;

        return Write(std::make_pair(std::string("key"), vchPubKey.Raw()), vchPrivKey, false);
    }

    bool WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata &keyMeta)
    {
        nWalletDBUpdated++;
        bool fEraseUnencryptedKey = true;

        if(!Write(std::make_pair(std::string("keymeta"), vchPubKey), keyMeta))
            return false;

        if (!Write(std::make_pair(std::string("ckey"), vchPubKey.Raw()), vchCryptedSecret, false))
            return false;
        if (fEraseUnencryptedKey)
        {
            Erase(std::make_pair(std::string("key"), vchPubKey.Raw()));
            Erase(std::make_pair(std::string("wkey"), vchPubKey.Raw()));
        }
        return true;
    }

    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey)
    {
        nWalletDBUpdated++;
        return Write(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
    }

    bool WriteCScript(const uint160& hash, const CScript& redeemScript)
    {
        nWalletDBUpdated++;
        return Write(std::make_pair(std::string("cscript"), hash), redeemScript, false);
    }

    bool WriteBestBlock(const CBlockLocator& locator)
    {
        nWalletDBUpdated++;
        return Write(std::string("bestblock"), locator);
    }

    bool ReadBestBlock(CBlockLocator& locator)
    {
        return Read(std::string("bestblock"), locator);
    }

    bool WriteOrderPosNext(int64_t nOrderPosNext)
    {
        nWalletDBUpdated++;
        return Write(std::string("orderposnext"), nOrderPosNext);
    }

	bool WriteStakeSplitThreshold(uint64_t nStakeSplitThreshold)
	{
		nWalletDBUpdated++;
		return Write(std::string("stakeSplitThreshold"), nStakeSplitThreshold);
	}

	bool WriteMultiSend(std::vector<std::pair<std::string, int> > vMultiSend)
	{
		nWalletDBUpdated++;
		bool ret = true;
		for(unsigned int i = 0; i < vMultiSend.size(); i++)
		{
			std::pair<std::string, int> pMultiSend;
			pMultiSend = vMultiSend[i];
			if(!Write(std::make_pair(std::string("multisend"), i), pMultiSend, true))
				ret = false;
		}
		return ret;
	}

	bool EraseMultiSend(std::vector<std::pair<std::string, int> > vMultiSend)
	{
		nWalletDBUpdated++;
		bool ret = true;
		for(unsigned int i = 0; i < vMultiSend.size(); i++)
		{
			std::pair<std::string, int> pMultiSend;
			pMultiSend = vMultiSend[i];
			if(!Erase(std::make_pair(std::string("multisend"), i)))
				ret = false;
		}
		return ret;
	}

	bool WriteMSettings(bool fEnable, int nLastMultiSendHeight)
	{
		nWalletDBUpdated++;
		std::pair<bool, int> pSettings;
		pSettings.first = fEnable;
		pSettings.second = nLastMultiSendHeight;
		return Write(std::string("msettings"), pSettings, true);
	}

	bool WriteCharitySettings(bool fEnable)
	{
		nWalletDBUpdated++;
		return Write(std::string("charitysettings"), fEnable, true);
	}
	bool WriteCharity(std::vector<StakeForCharityEntry> vCharity)
	{
		nWalletDBUpdated++;
		bool ret = true;
		for(unsigned int i = 0; i < vCharity.size(); i++)
		{
			if(!Write(std::make_pair(std::string("charity"), i), vCharity[i], true))
				ret = false;
		}
		return ret;
	}
	bool EraseCharity(std::vector<StakeForCharityEntry> vCharity)
	{
		nWalletDBUpdated++;
		bool ret = true;
		for(unsigned int i = 0; i < vCharity.size(); i++)
		{
			if(!Erase(std::make_pair(std::string("charity"), i)))
				ret = false;
		}
		return ret;
	}

	bool WriteMSDisabledAddresses(std::vector<std::string> vDisabledAddresses)
	{
		nWalletDBUpdated++;
		bool ret = true;
		for(unsigned int i = 0; i < vDisabledAddresses.size(); i++)
		{
			if(!Write(std::make_pair(std::string("mdisabled"), i), vDisabledAddresses[i]))
				ret = false;
		}
		return ret;
	}

	bool EraseMSDisabledAddresses(std::vector<std::string> vDisabledAddresses)
	{
		nWalletDBUpdated++;
		bool ret = true;
		for(unsigned int i = 0; i < vDisabledAddresses.size(); i++)
		{
			if(!Erase(std::make_pair(std::string("mdisabled"), i)))
				ret = false;
		}
		return ret;
	}

	bool WriteHashDrift(unsigned int nHashDrift)
	{
		nWalletDBUpdated++;
		return Write(std::string("hashdrift"), nHashDrift, true);
	}

	bool WriteHashInterval(unsigned int nHashInterval)
	{
		nWalletDBUpdated++;
		return Write(std::string("hashinterval"), nHashInterval, true);
	}

    bool WriteDefaultKey(const CPubKey& vchPubKey)
    {
        nWalletDBUpdated++;
        return Write(std::string("defaultkey"), vchPubKey.Raw());
    }

    bool ReadPool(int64_t nPool, CKeyPool& keypool)
    {
        return Read(std::make_pair(std::string("pool"), nPool), keypool);
    }

    bool WritePool(int64_t nPool, const CKeyPool& keypool)
    {
        nWalletDBUpdated++;
        return Write(std::make_pair(std::string("pool"), nPool), keypool);
    }

    bool ErasePool(int64_t nPool)
    {
        nWalletDBUpdated++;
        return Erase(std::make_pair(std::string("pool"), nPool));
    }

    template<typename T>
    bool ReadSetting(const std::string& strKey, T& value)
    {
        return Read(std::make_pair(std::string("setting"), strKey), value);
    }
    template<typename T>
    bool WriteSetting(const std::string& strKey, const T& value)
    {
        nWalletDBUpdated++;
        return Write(std::make_pair(std::string("setting"), strKey), value);
    }
    bool EraseSetting(const std::string& strKey)
    {
        nWalletDBUpdated++;
        return Erase(std::make_pair(std::string("setting"), strKey));
    }

    bool WriteMinVersion(int nVersion)
    {
        return Write(std::string("minversion"), nVersion);
    }

    bool ReadAccount(const std::string& strAccount, CAccount& account);
    bool WriteAccount(const std::string& strAccount, const CAccount& account);
private:
    bool WriteAccountingEntry(const uint64_t nAccEntryNum, const CAccountingEntry& acentry);
public:
    bool WriteAccountingEntry(const CAccountingEntry& acentry);
    int64_t GetAccountCreditDebit(const std::string& strAccount);
    void ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& acentries);

    DBErrors ReorderTransactions(CWallet*);
    DBErrors LoadWallet(CWallet* pwallet);
    static bool Recover(CDBEnv& dbenv, std::string filename, bool fOnlyKeys);
    static bool Recover(CDBEnv& dbenv, std::string filename);
};

#endif
