#ifdef ENABLE_MWEB

#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "main.h"
#include "init.h"
#include "base58.h"
#include "script.h"
#include "txdb.h"

#include "mw/mw_wallet.h"
#include "mw/mw_common.h"
#include "mw/peg.h"
#include "mw/state/mw_state.h"

#include <boost/foreach.hpp>

using namespace json_spirit;
using namespace std;

extern CWallet* pwalletMain;

extern mw::CMWState g_mwState;

extern mw::CMWWallet g_mwWallet;

static void EnsureMWWalletReady()
{
    if (!g_mwWallet.HasKeys())
    {

        if (!g_mwWallet.GenerateKeys())
            throw JSONRPCError(RPC_WALLET_ERROR, "Failed to generate MWEB keys");
    }
}

Value mweb_pegin(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "mweb_pegin <amount>\n"
            "Peg transparent coins into the MWEB (Mimblewimble Extension Block).\n"
            "<amount> is a real number (e.g. 10.0) and is rounded to the nearest 0.000001.\n"
            "The transparent coins are burned via an OP_RETURN transaction,\n"
            "and an equivalent MW output is created in the MWEB.\n"
            "Returns the transparent transaction id."
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();
    EnsureMWWalletReady();

    int64_t nAmount = AmountFromValue(params[0]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount must be positive");

    mw::CPegInResult result = mw::CreatePegIn(pwalletMain, nAmount);
    if (!result.fSuccess)
        throw JSONRPCError(RPC_WALLET_ERROR, "Peg-in failed: " + result.strError);

    mw::CMWOwnedOutput owned;
    owned.output = result.mwOutput;
    owned.blindingFactor = result.outputBlind;

    unsigned char scanPub[mw::PUBKEY_SIZE];
    g_mwWallet.GetScanPubKey(scanPub);

    owned.nValue = nAmount;
    owned.nBlockHeight = 0;
    owned.fSpent = false;
    g_mwWallet.AddOwnedOutput(owned);

    CWalletTx wtxPegIn(pwalletMain, result.transparentTx);
    CReserveKey reservekey(pwalletMain);
    if (!pwalletMain->CommitTransaction(wtxPegIn, reservekey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to broadcast peg-in transaction");

    Object obj;
    obj.push_back(Pair("txid", wtxPegIn.GetHash().GetHex()));
    obj.push_back(Pair("mw_commitment", HexStr(result.mwOutput.commitment.begin(),
                                                result.mwOutput.commitment.end())));
    obj.push_back(Pair("amount", ValueFromAmount(nAmount)));

    return obj;
}

Value mweb_pegout(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "mweb_pegout <amount> <address>\n"
            "Peg coins out from the MWEB to a transparent address.\n"
            "<amount> is a real number and is rounded to the nearest 0.000001.\n"
            "<address> is a transparent MaryJaneCoin address.\n"
            "Returns the MWEB kernel hash."
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();
    EnsureMWWalletReady();

    int64_t nAmount = AmountFromValue(params[0]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount must be positive");

    string strAddress = params[1].get_str();
    CBitcoinAddress address(strAddress);
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid MaryJaneCoin address");

    CTxDestination dest = address.Get();

    mw::CPegOutResult result = mw::CreatePegOut(pwalletMain, nAmount, dest);
    if (!result.fSuccess)
        throw JSONRPCError(RPC_WALLET_ERROR, "Peg-out failed: " + result.strError);

    g_mwWallet.MarkOutputSpent(result.mwInput.commitment);

    Object obj;
    obj.push_back(Pair("kernel_hash", result.kernel.GetHash().GetHex()));
    obj.push_back(Pair("destination", strAddress));
    obj.push_back(Pair("amount", ValueFromAmount(nAmount)));

    return obj;
}

Value mweb_send(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "mweb_send <scan_pubkey> <spend_pubkey> <amount>\n"
            "Send coins privately within the MWEB.\n"
            "<scan_pubkey>  — receiver's 33-byte scan public key (hex)\n"
            "<spend_pubkey> — receiver's 33-byte spend public key (hex)\n"
            "<amount>       — amount to send (real number)\n"
            "Returns the MW transaction hash."
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();
    EnsureMWWalletReady();

    vector<unsigned char> vchScanKey = ParseHexV(params[0], "scan_pubkey");
    if (vchScanKey.size() != mw::PUBKEY_SIZE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "scan_pubkey must be 33 bytes (compressed public key)");

    vector<unsigned char> vchSpendKey = ParseHexV(params[1], "spend_pubkey");
    if (vchSpendKey.size() != mw::PUBKEY_SIZE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "spend_pubkey must be 33 bytes (compressed public key)");

    int64_t nAmount = AmountFromValue(params[2]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount must be positive");

    mw::CMWTransaction mwtx;
    if (!g_mwWallet.CreateMWTransaction(vchScanKey.data(), vchSpendKey.data(), nAmount, mwtx))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to create MWEB transaction (insufficient funds?)");

    Object obj;
    obj.push_back(Pair("mw_txhash", mwtx.GetHash().GetHex()));
    obj.push_back(Pair("amount", ValueFromAmount(nAmount)));
    obj.push_back(Pair("inputs", (int)mwtx.body.vInputs.size()));
    obj.push_back(Pair("outputs", (int)mwtx.body.vOutputs.size()));
    obj.push_back(Pair("kernels", (int)mwtx.body.vKernels.size()));

    return obj;
}

Value mweb_balance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "mweb_balance\n"
            "Show the MWEB balance, separate from the transparent balance.\n"
            "Returns an object with total, confirmed, and unconfirmed MWEB balances.");

    EnsureMWWalletReady();

    Object obj;
    obj.push_back(Pair("total",       ValueFromAmount(g_mwWallet.GetMWBalance())));
    obj.push_back(Pair("confirmed",   ValueFromAmount(g_mwWallet.GetMWConfirmedBalance())));
    obj.push_back(Pair("unconfirmed", ValueFromAmount(g_mwWallet.GetMWUnconfirmedBalance())));
    obj.push_back(Pair("outputs",     (int)g_mwWallet.GetOutputCount()));

    return obj;
}

Value mweb_outputs(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "mweb_outputs [all]\n"
            "List all owned MWEB outputs.\n"
            "By default only shows unspent outputs.\n"
            "Pass \"all\" to include spent outputs.\n"
            "Returns an array of output objects.");

    EnsureMWWalletReady();

    bool fAll = false;
    if (params.size() > 0 && params[0].get_str() == "all")
        fAll = true;

    vector<mw::CMWOwnedOutput> vOutputs;
    if (fAll)
        vOutputs = g_mwWallet.GetOwnedOutputs();
    else
        vOutputs = g_mwWallet.GetUnspentOutputs();

    Array result;
    for (size_t i = 0; i < vOutputs.size(); i++)
    {
        const mw::CMWOwnedOutput& output = vOutputs[i];

        Object entry;
        entry.push_back(Pair("commitment", HexStr(output.output.commitment.begin(),
                                                   output.output.commitment.end())));
        entry.push_back(Pair("amount",     ValueFromAmount(output.nValue)));
        entry.push_back(Pair("height",     output.nBlockHeight));
        entry.push_back(Pair("spent",      output.fSpent));

        if (output.nBlockHeight > 0 && nBestHeight > 0)
        {
            int nConfs = nBestHeight - output.nBlockHeight + 1;
            entry.push_back(Pair("confirmations", nConfs));
        }
        else
        {
            entry.push_back(Pair("confirmations", 0));
        }

        entry.push_back(Pair("output_id", output.output.GetOutputID().GetHex()));

        result.push_back(entry);
    }

    return result;
}

Value mweb_info(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "mweb_info\n"
            "Show MWEB state information.\n"
            "Returns an object with height, output count, supply, MMR root, and wallet info.");

    Object obj;

    Object stateObj;
    stateObj.push_back(Pair("height",       g_mwState.GetHeight()));
    stateObj.push_back(Pair("output_count", (int)g_mwState.GetOutputCount()));
    stateObj.push_back(Pair("supply",       ValueFromAmount(g_mwState.GetMWEBSupply())));
    stateObj.push_back(Pair("mmr_root",     g_mwState.GetMMRRoot().GetHex()));
    stateObj.push_back(Pair("latest_block", g_mwState.GetLatestMWBlockHash().GetHex()));
    obj.push_back(Pair("state", stateObj));

    Object walletObj;
    walletObj.push_back(Pair("has_keys",    g_mwWallet.HasKeys()));
    walletObj.push_back(Pair("balance",     ValueFromAmount(g_mwWallet.GetMWBalance())));
    walletObj.push_back(Pair("outputs",     (int)g_mwWallet.GetOutputCount()));

    if (g_mwWallet.HasKeys())
    {
        unsigned char scanPub[mw::PUBKEY_SIZE];
        unsigned char spendPub[mw::PUBKEY_SIZE];
        g_mwWallet.GetScanPubKey(scanPub);
        g_mwWallet.GetSpendPubKey(spendPub);
        walletObj.push_back(Pair("scan_pubkey",  HexStr(scanPub, scanPub + mw::PUBKEY_SIZE)));
        walletObj.push_back(Pair("spend_pubkey", HexStr(spendPub, spendPub + mw::PUBKEY_SIZE)));
    }
    obj.push_back(Pair("wallet", walletObj));

    return obj;
}

Value mweb_verify(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "mweb_verify <commitment>\n"
            "Verify an MWEB commitment exists in the UTXO set and its range proof is valid.\n"
            "<commitment> is a 33-byte commitment in hex (66 hex characters).\n"
            "This verifies that the hidden amount is valid without revealing it.\n"
            "Returns verification status, output ID, and MMR proof root.");

    string strCommitment = params[0].get_str();
    vector<unsigned char> vchCommit = ParseHex(strCommitment);
    if (vchCommit.size() != mw::COMMITMENT_SIZE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "commitment must be 33 bytes (66 hex characters)");

    mw::Commitment commitment(vchCommit.data());

    Object obj;
    obj.push_back(Pair("commitment", strCommitment));

    mw::CMWOutput output;
    bool fExists = g_mwState.GetOutput(commitment, output);
    obj.push_back(Pair("exists", fExists));

    if (fExists)
    {

        bool fRangeProofValid = mw::crypto::BulletproofVerifier::Verify(
            output.commitment, output.rangeProof);
        obj.push_back(Pair("range_proof_valid", fRangeProofValid));
        obj.push_back(Pair("range_proof_size", (int)output.rangeProof.GetSize()));

        obj.push_back(Pair("output_id", output.GetOutputID().GetHex()));
        obj.push_back(Pair("is_coinbase", output.IsCoinbase()));
        obj.push_back(Pair("sender_pubkey", HexStr(output.senderPubKey,
                                                     output.senderPubKey + mw::PUBKEY_SIZE)));
        obj.push_back(Pair("receiver_pubkey", HexStr(output.receiverPubKey,
                                                       output.receiverPubKey + mw::PUBKEY_SIZE)));

        mw::CMMRProof proof = g_mwState.GetOutputProof(commitment);
        obj.push_back(Pair("mmr_proof_available", !proof.IsNull()));
        if (!proof.IsNull())
        {
            bool fMMRValid = g_mwState.VerifyOutputProof(commitment, proof);
            obj.push_back(Pair("mmr_proof_valid", fMMRValid));
            obj.push_back(Pair("mmr_root", g_mwState.GetMMRRoot().GetHex()));
        }

        bool fOurs = false;
        vector<mw::CMWOwnedOutput> vOwned = g_mwWallet.GetOwnedOutputs();
        for (size_t i = 0; i < vOwned.size(); i++)
        {
            if (vOwned[i].output.commitment == commitment)
            {
                fOurs = true;
                obj.push_back(Pair("is_ours", true));
                obj.push_back(Pair("our_value", ValueFromAmount(vOwned[i].nValue)));
                obj.push_back(Pair("our_spent", vOwned[i].fSpent));
                obj.push_back(Pair("our_height", vOwned[i].nBlockHeight));
                break;
            }
        }
        if (!fOurs)
            obj.push_back(Pair("is_ours", false));
    }
    else
    {
        obj.push_back(Pair("message", "Commitment not found in MWEB UTXO set (may be spent or not yet confirmed)"));
    }

    return obj;
}

#endif
