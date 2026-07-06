#include "bitcoinrpc.h"
#include "payjoin.h"
#include "init.h"
#include "wallet.h"
#include "util.h"

#include <boost/foreach.hpp>

using namespace json_spirit;
using namespace std;

Value payjoin(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "payjoin <address> <amount> [endpoint]\n"
            "Send <amount> MARYJ to <address> using PayJoin (BIP78) for transaction privacy.\n"
            "\n"
            "PayJoin breaks the 'common input ownership' heuristic by having both\n"
            "sender and receiver contribute inputs to the transaction. The result\n"
            "looks like a normal transaction on-chain but chain analysis cannot\n"
            "determine which inputs belong to which party.\n"
            "\n"
            "If [endpoint] is provided (e.g. 'localhost:8422'), the sender contacts\n"
            "the receiver's PayJoin server to collaboratively build the transaction.\n"
            "If [endpoint] is omitted, a normal (non-PayJoin) transaction is sent.\n"
            "\n"
            "Returns: {txid, sender_inputs, receiver_inputs, total_inputs, outputs,\n"
            "          amount, fee, payjoin_used}\n"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
            "Error: Please enter the wallet passphrase with walletpassphrase first.");
    if (pwalletMain->fWalletUnlockMintOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
            "Error: Wallet unlocked for block minting only.");

    string strAddress = params[0].get_str();
    int64_t nAmount = AmountFromValue(params[1]);

    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

    CBitcoinAddress address(strAddress);
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid MaryJaneCoin address");

    CPayJoinClient client(pwalletMain);

    if (params.size() == 3)
    {

        string strEndpoint = params[2].get_str();
        CPayJoinResult result = client.SendPayJoin(strAddress, nAmount, strEndpoint);

        if (!result.fSuccess)
            throw JSONRPCError(RPC_WALLET_ERROR, result.strError);

        Object obj;
        obj.push_back(Pair("txid",             result.txHash.GetHex()));
        obj.push_back(Pair("sender_inputs",    result.nSenderInputs));
        obj.push_back(Pair("receiver_inputs",  result.nReceiverInputs));
        obj.push_back(Pair("total_inputs",     result.nTotalInputs));
        obj.push_back(Pair("outputs",          result.nOutputs));
        obj.push_back(Pair("amount",           ValueFromAmount(result.nAmount)));
        obj.push_back(Pair("fee",              ValueFromAmount(result.nFeePaid)));
        obj.push_back(Pair("payjoin_used",     result.fPayJoinUsed));

        if (!result.strError.empty() && result.fSuccess)
            obj.push_back(Pair("warning", result.strError));

        return obj;
    }
    else
    {

        CWalletTx wtxNew;
        string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtxNew);

        if (!strError.empty())
            throw JSONRPCError(RPC_WALLET_ERROR, strError);

        Object obj;
        obj.push_back(Pair("txid",             wtxNew.GetHash().GetHex()));
        obj.push_back(Pair("sender_inputs",    (int)wtxNew.vin.size()));
        obj.push_back(Pair("receiver_inputs",  0));
        obj.push_back(Pair("total_inputs",     (int)wtxNew.vin.size()));
        obj.push_back(Pair("outputs",          (int)wtxNew.vout.size()));
        obj.push_back(Pair("amount",           ValueFromAmount(nAmount)));
        obj.push_back(Pair("fee",              ValueFromAmount(0)));
        obj.push_back(Pair("payjoin_used",     false));
        obj.push_back(Pair("warning",          "No endpoint provided — sent as normal transaction"));
        return obj;
    }
}

Value payjoinreceive(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "payjoinreceive <port>\n"
            "Start the PayJoin receiver server on <port>.\n"
            "\n"
            "The server listens for POST /payjoin HTTP requests containing\n"
            "hex-encoded transactions. When a valid proposal arrives, the server\n"
            "automatically selects wallet UTXOs to add as receiver inputs,\n"
            "adjusts outputs, signs the receiver inputs, and returns the\n"
            "modified transaction for the sender to co-sign and broadcast.\n"
            "\n"
            "The wallet must be unlocked for the server to create proposals.\n"
            "\n"
            "Example: payjoinreceive 8422\n"
            + HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
            "Error: Please enter the wallet passphrase with walletpassphrase first.");

    int nPort = params[0].get_int();
    if (nPort <= 0 || nPort > 65535)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Port must be between 1 and 65535");

    if (pPayJoinServer && pPayJoinServer->IsRunning())
        throw JSONRPCError(RPC_MISC_ERROR,
            strprintf("PayJoin server is already running on port %d. Use payjoinstop first.",
                      pPayJoinServer->GetPort()));

    if (pPayJoinServer)
    {
        delete pPayJoinServer;
        pPayJoinServer = NULL;
    }

    pPayJoinServer = new CPayJoinServer(pwalletMain);
    string strError;
    if (!pPayJoinServer->Start(nPort, strError))
    {
        delete pPayJoinServer;
        pPayJoinServer = NULL;
        throw JSONRPCError(RPC_MISC_ERROR, strError);
    }

    Object obj;
    obj.push_back(Pair("status",   "running"));
    obj.push_back(Pair("port",     nPort));
    obj.push_back(Pair("endpoint", strprintf("http://localhost:%d/payjoin", nPort)));
    return obj;
}

Value payjoinstop(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "payjoinstop\n"
            "Stop the PayJoin receiver server.\n"
            "Returns the final status including how many proposals were processed.");

    if (!pPayJoinServer || !pPayJoinServer->IsRunning())
        throw JSONRPCError(RPC_MISC_ERROR, "PayJoin server is not running");

    int nPort = pPayJoinServer->GetPort();
    int nProposals = pPayJoinServer->GetProposalCount();

    pPayJoinServer->Stop();
    delete pPayJoinServer;
    pPayJoinServer = NULL;

    Object obj;
    obj.push_back(Pair("status",     "stopped"));
    obj.push_back(Pair("port",       nPort));
    obj.push_back(Pair("proposals",  nProposals));
    return obj;
}

Value payjoininfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "payjoininfo\n"
            "Show PayJoin status information.\n"
            "\n"
            "Returns:\n"
            "  server_running  - whether the PayJoin receiver is active\n"
            "  server_port     - port the receiver is listening on (0 if not running)\n"
            "  proposals       - number of PayJoin proposals processed as receiver\n"
            "  history_count   - total PayJoin transactions in session history\n"
            "  history         - array of recent PayJoin transactions\n");

    Object obj;

    bool fServerRunning = (pPayJoinServer && pPayJoinServer->IsRunning());
    obj.push_back(Pair("server_running", fServerRunning));
    obj.push_back(Pair("server_port",    fServerRunning ? pPayJoinServer->GetPort() : 0));
    obj.push_back(Pair("proposals",      fServerRunning ? pPayJoinServer->GetProposalCount() : 0));

    if (fServerRunning)
    {
        obj.push_back(Pair("endpoint",
            strprintf("http://localhost:%d/payjoin", pPayJoinServer->GetPort())));
    }

    vector<CPayJoinHistoryEntry> vHistory = GetPayJoinHistory();
    obj.push_back(Pair("history_count", (int)vHistory.size()));

    Array historyArray;
    BOOST_FOREACH(const CPayJoinHistoryEntry& entry, vHistory)
    {
        Object h;
        h.push_back(Pair("txid",             entry.txHash.GetHex()));
        h.push_back(Pair("time",             entry.nTimestamp));
        h.push_back(Pair("amount",           ValueFromAmount(entry.nAmount)));
        h.push_back(Pair("role",             entry.fSender ? "sender" : "receiver"));
        h.push_back(Pair("sender_inputs",    entry.nSenderInputs));
        h.push_back(Pair("receiver_inputs",  entry.nReceiverInputs));
        h.push_back(Pair("counterparty",     entry.strCounterparty));
        historyArray.push_back(h);
    }
    obj.push_back(Pair("history", historyArray));

    int nTotalTx = (int)vHistory.size();
    int nPayJoinTx = 0;
    BOOST_FOREACH(const CPayJoinHistoryEntry& entry, vHistory)
    {
        if (entry.nReceiverInputs > 0)
            nPayJoinTx++;
    }
    obj.push_back(Pair("privacy_score",
        nTotalTx > 0 ? strprintf("%d/%d PayJoin transactions (%.0f%%)",
                                   nPayJoinTx, nTotalTx,
                                   (double)nPayJoinTx / nTotalTx * 100.0)
                      : "No PayJoin transactions yet"));

    return obj;
}
