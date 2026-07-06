#include "coinjoin.h"
#include "wallet.h"
#include "base58.h"
#include "init.h"
#include "util.h"
#include "script.h"

#include <boost/foreach.hpp>

using namespace std;

CCoinJoinMixer::CCoinJoinMixer(CWallet* wallet)
    : pWallet(wallet), nStatus(COINJOIN_IDLE)
{
}

CoinJoinStatus CCoinJoinMixer::GetStatus() const
{
    LOCK(cs_coinjoin);
    return nStatus;
}

int64_t CCoinJoinMixer::FindBestDenomination(int64_t nAmount)
{

    for (int i = 0; i < COINJOIN_NUM_DENOMINATIONS; i++)
    {
        int64_t denom = COINJOIN_DENOMINATIONS[i];
        if (nAmount >= denom * COINJOIN_MIN_OUTPUTS)
            return denom;
    }
    return 0;
}

bool CCoinJoinMixer::SelectMixInputs(int64_t nDenomination, int nMinOutputs,
                                      set<pair<const CWalletTx*, unsigned int> >& setCoins,
                                      int64_t& nValueIn)
{

    int64_t nEstFee = 1000000LL;
    int64_t nTargetValue = nDenomination * nMinOutputs + nEstFee;

    vector<COutput> vCoins;
    pWallet->AvailableCoins(vCoins, true);

    return pWallet->SelectCoinsMinConf(nTargetValue, GetTime(), 1, 6, vCoins, setCoins, nValueIn);
}

CCoinJoinResult CCoinJoinMixer::MixAmount(int64_t nAmount)
{
    LOCK(cs_coinjoin);
    CCoinJoinResult result;

    if (!pWallet)
    {
        result.error = "Wallet not available";
        nStatus = COINJOIN_ERROR;
        return result;
    }

    if (pWallet->IsLocked())
    {
        result.error = "Wallet is locked. Please unlock with walletpassphrase first.";
        nStatus = COINJOIN_ERROR;
        return result;
    }

    nStatus = COINJOIN_SELECTING;

    int64_t nDenomination = FindBestDenomination(nAmount);
    if (nDenomination == 0)
    {
        result.error = strprintf("Amount too small to mix. Need at least %s MARYJ for minimum %d outputs.",
                                  FormatMoney(COINJOIN_DENOMINATIONS[COINJOIN_NUM_DENOMINATIONS - 1] * COINJOIN_MIN_OUTPUTS).c_str(),
                                  COINJOIN_MIN_OUTPUTS);
        nStatus = COINJOIN_ERROR;
        return result;
    }

    int nOutputs = (int)(nAmount / nDenomination);
    if (nOutputs > COINJOIN_MAX_OUTPUTS)
        nOutputs = COINJOIN_MAX_OUTPUTS;
    if (nOutputs < COINJOIN_MIN_OUTPUTS)
    {
        result.error = strprintf("Insufficient amount. Need at least %d outputs of denomination %s.",
                                  COINJOIN_MIN_OUTPUTS, FormatMoney(nDenomination).c_str());
        nStatus = COINJOIN_ERROR;
        return result;
    }

    int64_t nTotalOutputValue = nDenomination * nOutputs;

    nStatus = COINJOIN_MIXING;

    vector<pair<CScript, int64_t> > vecSend;

    if (!pWallet->IsLocked())
        pWallet->TopUpKeyPool();

    for (int i = 0; i < nOutputs; i++)
    {
        CPubKey newKey;
        if (!pWallet->GetKeyFromPool(newKey, false))
        {
            result.error = "Keypool ran out. Please call keypoolrefill first.";
            nStatus = COINJOIN_ERROR;
            return result;
        }
        CKeyID keyID = newKey.GetID();
        pWallet->SetAddressBookName(keyID, "automix");

        CScript scriptPubKey;
        scriptPubKey.SetDestination(keyID);
        vecSend.push_back(make_pair(scriptPubKey, nDenomination));
    }

    int64_t nBalance = pWallet->GetBalance();
    if (nBalance < nTotalOutputValue)
    {
        result.error = strprintf("Insufficient funds. Have %s, need %s for %d outputs.",
                                  FormatMoney(nBalance).c_str(),
                                  FormatMoney(nTotalOutputValue).c_str(),
                                  nOutputs);
        nStatus = COINJOIN_ERROR;
        return result;
    }

    CReserveKey reservekey(pWallet);
    CWalletTx wtx;
    wtx.mapValue["comment"] = "coinjoin";
    int64_t nFeeRequired = 0;

    bool fCreated = pWallet->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, 1);
    if (!fCreated)
    {
        if (nTotalOutputValue + nFeeRequired > nBalance)
            result.error = strprintf("Insufficient funds after fee. Need %s (including %s fee).",
                                      FormatMoney(nTotalOutputValue + nFeeRequired).c_str(),
                                      FormatMoney(nFeeRequired).c_str());
        else
            result.error = "Transaction creation failed";
        nStatus = COINJOIN_ERROR;
        return result;
    }

    if (!pWallet->CommitTransaction(wtx, reservekey))
    {
        result.error = "Transaction commit failed";
        nStatus = COINJOIN_ERROR;
        return result;
    }

    result.txHash = wtx.GetHash();
    result.numInputs = (int)wtx.vin.size();
    result.numOutputs = nOutputs;
    result.denomination = nDenomination;
    result.totalMixed = nTotalOutputValue;

    int64_t nTotalIn = 0;
    BOOST_FOREACH(const CTxIn& txin, wtx.vin)
    {

        map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
        if (mi != pWallet->mapWallet.end())
        {
            const CWalletTx& wtxPrev = mi->second;
            if (txin.prevout.n < wtxPrev.vout.size())
                nTotalIn += wtxPrev.vout[txin.prevout.n].nValue;
        }
    }
    int64_t nTotalOut = 0;
    BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        nTotalOut += txout.nValue;
    result.feePaid = nTotalIn - nTotalOut;
    result.success = true;

    nStatus = COINJOIN_COMPLETE;
    return result;
}

CCoinJoinResult CCoinJoinMixer::AutoMix()
{
    CCoinJoinResult result;

    if (!pWallet)
    {
        result.error = "Wallet not available";
        return result;
    }

    int64_t nBalance = pWallet->GetBalance();
    if (nBalance <= 0)
    {
        result.error = "No available balance to mix";
        return result;
    }

    int64_t nBestDenom = FindBestDenomination(nBalance);
    if (nBestDenom == 0)
    {
        result.error = strprintf("Balance too low to mix. Need at least %s MARYJ.",
                                  FormatMoney(COINJOIN_DENOMINATIONS[COINJOIN_NUM_DENOMINATIONS - 1] * COINJOIN_MIN_OUTPUTS).c_str());
        return result;
    }

    int nOutputs = (int)(nBalance / nBestDenom);
    if (nOutputs > COINJOIN_MAX_OUTPUTS)
        nOutputs = COINJOIN_MAX_OUTPUTS;

    int64_t nMixAmount = nBestDenom * nOutputs;
    return MixAmount(nMixAmount);
}

vector<pair<int64_t, int> > CCoinJoinMixer::GetAvailableMixes()
{
    vector<pair<int64_t, int> > vResult;

    int64_t nBalance = 0;
    if (pWallet)
        nBalance = pWallet->GetBalance();

    for (int i = 0; i < COINJOIN_NUM_DENOMINATIONS; i++)
    {
        int64_t denom = COINJOIN_DENOMINATIONS[i];
        int nOutputs = 0;
        if (nBalance >= denom)
        {
            nOutputs = (int)(nBalance / denom);
            if (nOutputs > COINJOIN_MAX_OUTPUTS)
                nOutputs = COINJOIN_MAX_OUTPUTS;
        }
        vResult.push_back(make_pair(denom, nOutputs));
    }

    return vResult;
}
