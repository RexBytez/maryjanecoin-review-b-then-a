#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "main.h"
#include "init.h"
#include "base58.h"
#include "stealth.h"
#include "script.h"

#include <boost/foreach.hpp>

using namespace json_spirit;
using namespace std;

extern CWallet* pwalletMain;

Value getpoolbalances(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpoolbalances\n"
            "Returns the wallet balance split between transparent and shielded pools.\n"
            "\nResult:\n"
            "{\n"
            "  \"transparent\" : n,      (numeric) Balance in transparent pool (for staking)\n"
            "  \"shielded\" : n,         (numeric) Balance in shielded pool (for spending)\n"
            "  \"total\" : n,            (numeric) Total balance (transparent + shielded)\n"
            "  \"two_pool_active\" : b,  (boolean) Whether two-pool consensus is active\n"
            "  \"activation_height\" : n (numeric) Height at which two-pool activates\n"
            "}\n");

    int64_t nTransparent = pwalletMain->GetTransparentBalance();
    int64_t nShielded = pwalletMain->GetShieldedBalance();

    Object result;
    result.push_back(Pair("transparent",       ValueFromAmount(nTransparent)));
    result.push_back(Pair("shielded",          ValueFromAmount(nShielded)));
    result.push_back(Pair("total",             ValueFromAmount(nTransparent + nShielded)));
    result.push_back(Pair("two_pool_active",   nBestHeight >= TWO_POOL_ACTIVATION_HEIGHT));
    result.push_back(Pair("activation_height", TWO_POOL_ACTIVATION_HEIGHT));

    return result;
}

Value pegin(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "pegin <amount>\n"
            "Move <amount> coins from the transparent pool to the shielded pool.\n"
            "Creates a stealth send to your own stealth address.\n"
            "After this, the coins are available for private spending but not staking.\n"
            "<amount> is a real number and is rounded to the nearest 0.000001.\n"
            "Returns the transaction id."
            + HelpRequiringPassphrase());

    if (nBestHeight < TWO_POOL_ACTIVATION_HEIGHT)
        throw JSONRPCError(RPC_MISC_ERROR,
            strprintf("Two-pool system not yet active (current height %d, activates at %d)",
                nBestHeight, TWO_POOL_ACTIVATION_HEIGHT));

    EnsureWalletIsUnlocked();

    int64_t nAmount = AmountFromValue(params[0]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount must be positive");

    int64_t nTransparent = pwalletMain->GetTransparentBalance();
    if (nAmount > nTransparent)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS,
            strprintf("Insufficient transparent pool balance (have %s, need %s)",
                FormatMoney(nTransparent).c_str(), FormatMoney(nAmount).c_str()));

    vector<CStealthAddress> vStealth;
    pwalletMain->GetStealthAddresses(vStealth);

    CStealthAddress sxAddr;
    if (vStealth.empty())
    {

        CKey scanSecret;
        CKey spendSecret;
        if (!GenerateStealthAddress(sxAddr, scanSecret, spendSecret))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to generate stealth address for pegin");

        sxAddr.label = "pegin-auto";

        if (!pwalletMain->AddKey(scanSecret))
            throw JSONRPCError(RPC_WALLET_ERROR, "Failed to store scan key");
        if (!pwalletMain->AddKey(spendSecret))
            throw JSONRPCError(RPC_WALLET_ERROR, "Failed to store spend key");
        if (!pwalletMain->AddStealthAddress(sxAddr))
            throw JSONRPCError(RPC_WALLET_ERROR, "Failed to store stealth address");

        printf("pegin: auto-generated stealth address %s\n", sxAddr.Encoded().c_str());
    }
    else
    {

        sxAddr = vStealth[0];
    }

    CStealthPayment payment;
    if (!PrepareStealthPayment(sxAddr, payment))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to prepare stealth payment for pegin");

    CScript scriptDest;
    scriptDest.SetDestination(payment.destKeyID);

    const vector<unsigned char>& vchEphemeral = payment.ephemeralPubKey.Raw();
    if (vchEphemeral.size() != 33)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Ephemeral pubkey is not 33 bytes");

    CScript scriptOpReturn;
    scriptOpReturn << OP_RETURN << vchEphemeral;

    vector<pair<CScript, int64_t> > vecSend;
    vecSend.push_back(make_pair(scriptDest,     nAmount));
    vecSend.push_back(make_pair(scriptOpReturn, 0));

    CWalletTx wtx;
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;

    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, 1);
    if (!fCreated)
    {
        if (nAmount + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds (including fee)");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }

    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    printf("pegin: moved %s from transparent to shielded pool, txid=%s\n",
        FormatMoney(nAmount).c_str(), wtx.GetHash().GetHex().c_str());

    return wtx.GetHash().GetHex();
}

Value pegout(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "pegout <amount>\n"
            "Move <amount> coins from the shielded pool to the transparent pool.\n"
            "Creates a transparent output with a PEGOUT marker.\n"
            "The resulting coins are available for staking but not private spending.\n"
            "<amount> is a real number and is rounded to the nearest 0.000001.\n"
            "Returns the transaction id."
            + HelpRequiringPassphrase());

    if (nBestHeight < TWO_POOL_ACTIVATION_HEIGHT)
        throw JSONRPCError(RPC_MISC_ERROR,
            strprintf("Two-pool system not yet active (current height %d, activates at %d)",
                nBestHeight, TWO_POOL_ACTIVATION_HEIGHT));

    EnsureWalletIsUnlocked();

    int64_t nAmount = AmountFromValue(params[0]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount must be positive");

    int64_t nShielded = pwalletMain->GetShieldedBalance();
    if (nAmount > nShielded)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS,
            strprintf("Insufficient shielded pool balance (have %s, need %s)",
                FormatMoney(nShielded).c_str(), FormatMoney(nAmount).c_str()));

    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    CScript scriptDest;
    scriptDest.SetDestination(keyID);

    CScript scriptPegout;
    vector<unsigned char> vchPegout(6);
    vchPegout[0] = 'P'; vchPegout[1] = 'E'; vchPegout[2] = 'G';
    vchPegout[3] = 'O'; vchPegout[4] = 'U'; vchPegout[5] = 'T';
    scriptPegout << OP_RETURN << vchPegout;

    vector<pair<CScript, int64_t> > vecSend;
    vecSend.push_back(make_pair(scriptDest,    nAmount));
    vecSend.push_back(make_pair(scriptPegout,  0));

    CWalletTx wtx;
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;

    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, 1);
    if (!fCreated)
    {
        if (nAmount + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds (including fee)");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }

    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    printf("pegout: moved %s from shielded to transparent pool, txid=%s\n",
        FormatMoney(nAmount).c_str(), wtx.GetHash().GetHex().c_str());

    pwalletMain->SetAddressBookName(keyID, "pegout-staking");

    return wtx.GetHash().GetHex();
}
