#include "bitcoinrpc.h"
#include "coinjoin.h"
#include "init.h"
#include "wallet.h"
#include "util.h"

#include <boost/foreach.hpp>

using namespace json_spirit;
using namespace std;

Value coinjoin(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "coinjoin <amount>\n"
            "Mix <amount> MARYJ into equal-denomination outputs sent to fresh wallet addresses.\n"
            "Creates a self-mix (churn) transaction that breaks transaction graph linkage.\n"
            "The denomination is chosen automatically as the largest that gives at least 3 outputs.\n"
            "All outputs are exactly the same amount — this is the core privacy property.\n"
            "Returns: {txid, inputs, outputs, denomination, total_mixed, fee}\n"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
            "Error: Please enter the wallet passphrase with walletpassphrase first.");
    if (pwalletMain->fWalletUnlockMintOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
            "Error: Wallet unlocked for block minting only.");

    int64_t nAmount = AmountFromValue(params[0]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

    CCoinJoinMixer mixer(pwalletMain);
    CCoinJoinResult result = mixer.MixAmount(nAmount);

    if (!result.success)
        throw JSONRPCError(RPC_WALLET_ERROR, result.error);

    Object obj;
    obj.push_back(Pair("txid",        result.txHash.GetHex()));
    obj.push_back(Pair("inputs",      result.numInputs));
    obj.push_back(Pair("outputs",     result.numOutputs));
    obj.push_back(Pair("denomination", ValueFromAmount(result.denomination)));
    obj.push_back(Pair("total_mixed", ValueFromAmount(result.totalMixed)));
    obj.push_back(Pair("fee",         ValueFromAmount(result.feePaid)));
    return obj;
}

Value coinjoinauto(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "coinjoinauto\n"
            "Automatically mix the maximum amount using the best denomination.\n"
            "Selects the largest denomination that produces at least 3 outputs from the current balance.\n"
            "Returns: {txid, inputs, outputs, denomination, total_mixed, fee}\n"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
            "Error: Please enter the wallet passphrase with walletpassphrase first.");
    if (pwalletMain->fWalletUnlockMintOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
            "Error: Wallet unlocked for block minting only.");

    CCoinJoinMixer mixer(pwalletMain);
    CCoinJoinResult result = mixer.AutoMix();

    if (!result.success)
        throw JSONRPCError(RPC_WALLET_ERROR, result.error);

    Object obj;
    obj.push_back(Pair("txid",        result.txHash.GetHex()));
    obj.push_back(Pair("inputs",      result.numInputs));
    obj.push_back(Pair("outputs",     result.numOutputs));
    obj.push_back(Pair("denomination", ValueFromAmount(result.denomination)));
    obj.push_back(Pair("total_mixed", ValueFromAmount(result.totalMixed)));
    obj.push_back(Pair("fee",         ValueFromAmount(result.feePaid)));
    return obj;
}

Value coinjoininfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "coinjoininfo\n"
            "Show available CoinJoin denominations and how many outputs each could produce\n"
            "from the current wallet balance.\n"
            "A denomination is usable if it produces at least 3 outputs (COINJOIN_MIN_OUTPUTS).\n"
            "Returns: {balance, min_outputs, max_outputs, denominations: [{amount, outputs, usable}]}");

    int64_t nBalance = pwalletMain->GetBalance();

    CCoinJoinMixer mixer(pwalletMain);
    vector<pair<int64_t, int> > vMixes = mixer.GetAvailableMixes();

    Array denomArray;
    typedef pair<int64_t, int> MixPair;
    BOOST_FOREACH(const MixPair& p, vMixes)
    {
        Object d;
        d.push_back(Pair("amount",   ValueFromAmount(p.first)));
        d.push_back(Pair("outputs",  p.second));
        d.push_back(Pair("usable",   p.second >= COINJOIN_MIN_OUTPUTS));
        denomArray.push_back(d);
    }

    Object obj;
    obj.push_back(Pair("balance",      ValueFromAmount(nBalance)));
    obj.push_back(Pair("min_outputs",  COINJOIN_MIN_OUTPUTS));
    obj.push_back(Pair("max_outputs",  COINJOIN_MAX_OUTPUTS));
    obj.push_back(Pair("denominations", denomArray));
    return obj;
}
