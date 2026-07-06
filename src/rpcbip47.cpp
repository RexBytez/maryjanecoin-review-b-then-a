#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "main.h"
#include "init.h"
#include "base58.h"
#include "bip47.h"
#include "script.h"
#include "txdb.h"

#include <boost/foreach.hpp>

using namespace json_spirit;
using namespace std;

extern CWallet* pwalletMain;

static bool GetOrCreatePaymentCode(CPaymentCode& codeOut,
                                   CKey& scanSecretOut,
                                   CKey& spendSecretOut)
{
    LOCK(pwalletMain->cs_wallet);

    if (!pwalletMain->mapPaymentChannels.empty())
    {

    }

    for (map<string, CPaymentChannel>::const_iterator it = pwalletMain->mapPaymentChannels.begin();
         it != pwalletMain->mapPaymentChannels.end(); ++it)
    {
        if (it->first == "__self__")
        {
            codeOut = it->second.theirCode;

            CKeyID scanKeyID  = codeOut.scanPubKey.GetID();
            CKeyID spendKeyID = codeOut.spendPubKey.GetID();

            if (pwalletMain->GetKey(scanKeyID, scanSecretOut) &&
                pwalletMain->GetKey(spendKeyID, spendSecretOut))
            {
                return true;
            }

            break;
        }
    }

    scanSecretOut.MakeNewKey(true);
    spendSecretOut.MakeNewKey(true);

    if (!scanSecretOut.IsValid() || !spendSecretOut.IsValid())
        return false;

    codeOut = CPaymentCode(scanSecretOut.GetPubKey(), spendSecretOut.GetPubKey());
    codeOut.label = "__bip47__";

    if (!codeOut.IsValid())
        return false;

    if (!pwalletMain->AddKey(scanSecretOut))
        return false;
    if (!pwalletMain->AddKey(spendSecretOut))
        return false;

    CPaymentChannel selfChannel;
    selfChannel.theirCode = codeOut;
    selfChannel.nCreateTime = GetTime();
    pwalletMain->AddPaymentChannel("__self__", selfChannel);

    return true;
}

Value getpaymentcode(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpaymentcode\n"
            "Returns this wallet's BIP47 reusable payment code.\n"
            "Anyone who knows this code can establish a payment channel\n"
            "and send private payments without interactive address exchange.\n"
            "The code is base58check-encoded with version byte 0x47.");

    EnsureWalletIsUnlocked();

    CPaymentCode code;
    CKey scanSecret, spendSecret;
    if (!GetOrCreatePaymentCode(code, scanSecret, spendSecret))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to generate payment code");

    Object result;
    result.push_back(Pair("payment_code", code.ToBase58()));
    result.push_back(Pair("notification_address",
                     CBitcoinAddress(code.GetNotificationAddress()).ToString()));

    const vector<unsigned char>& vchScan  = code.scanPubKey.Raw();
    const vector<unsigned char>& vchSpend = code.spendPubKey.Raw();
    result.push_back(Pair("scan_pubkey",  HexStr(vchScan.begin(),  vchScan.end())));
    result.push_back(Pair("spend_pubkey", HexStr(vchSpend.begin(), vchSpend.end())));

    return result;
}

Value sendtonotify(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "sendtonotify <payment_code>\n"
            "Send a notification transaction to establish a BIP47 payment channel.\n"
            "This reveals your payment code to the recipient so they can derive\n"
            "shared addresses for receiving payments from you.\n"
            "Returns the transaction id."
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();

    string strTheirCode = params[0].get_str();
    CPaymentCode theirCode;
    if (!theirCode.FromBase58(strTheirCode))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BIP47 payment code");

    CPaymentCode ourCode;
    CKey scanSecret, spendSecret;
    if (!GetOrCreatePaymentCode(ourCode, scanSecret, spendSecret))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get wallet payment code");

    string strChannelKey = theirCode.ToBase58();
    {
        LOCK(pwalletMain->cs_wallet);
        map<string, CPaymentChannel>::iterator it =
            pwalletMain->mapPaymentChannels.find(strChannelKey);
        if (it != pwalletMain->mapPaymentChannels.end() && it->second.fNotificationSent)
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                "Notification already sent to this payment code. "
                "Use existing channel to send payments.");
    }

    uint256 blindingSecret;
    if (!ComputeBlindingSecret(scanSecret, theirCode.scanPubKey, blindingSecret))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to compute blinding secret");

    vector<unsigned char> vchPayload;
    if (!ourCode.Encode(vchPayload))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to encode payment code");

    vector<unsigned char> vchBlinded;
    if (!BlindPaymentCode(vchPayload, blindingSecret, vchBlinded))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to blind payment code");

    CKeyID notificationAddr = theirCode.GetNotificationAddress();
    CScript scriptDest;
    scriptDest.SetDestination(notificationAddr);

    CScript scriptOpReturn = BuildNotificationScript(vchBlinded);
    if (scriptOpReturn.empty())
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to build notification script");

    vector<pair<CScript, int64_t> > vecSend;
    vecSend.push_back(make_pair(scriptDest, BIP47_NOTIFICATION_DUST));
    vecSend.push_back(make_pair(scriptOpReturn, 0));

    CWalletTx wtx;
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;

    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, 1);
    if (!fCreated)
    {
        if (BIP47_NOTIFICATION_DUST + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }

    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    uint256 channelSecret;
    if (!ComputeBIP47SharedSecret(scanSecret, theirCode.spendPubKey, channelSecret))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to compute channel shared secret");

    CPaymentChannel channel;
    channel.theirCode = theirCode;
    channel.sharedSecret = channelSecret;
    channel.fNotificationSent = true;
    channel.nCreateTime = GetTime();

    pwalletMain->AddPaymentChannel(strChannelKey, channel);

    Object result;
    result.push_back(Pair("txid", wtx.GetHash().GetHex()));
    result.push_back(Pair("notification_address",
                     CBitcoinAddress(notificationAddr).ToString()));
    result.push_back(Pair("channel_established", true));

    return result;
}

Value listpaymentcodes(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "listpaymentcodes\n"
            "List all BIP47 payment code relationships.\n"
            "Returns an array of objects with channel info.");

    Array result;

    LOCK(pwalletMain->cs_wallet);
    for (map<string, CPaymentChannel>::const_iterator it = pwalletMain->mapPaymentChannels.begin();
         it != pwalletMain->mapPaymentChannels.end(); ++it)
    {

        if (it->first == "__self__")
            continue;

        const CPaymentChannel& channel = it->second;
        Object entry;

        entry.push_back(Pair("payment_code",       it->first));
        entry.push_back(Pair("label",              channel.theirCode.label));
        entry.push_back(Pair("notification_sent",  channel.fNotificationSent));
        entry.push_back(Pair("notification_recv",  channel.fNotificationRecv));
        entry.push_back(Pair("next_send_index",    (int)channel.nNextSendIndex));
        entry.push_back(Pair("next_recv_index",    (int)channel.nNextRecvIndex));
        entry.push_back(Pair("created",            (int64_t)channel.nCreateTime));

        CKeyID notifAddr = channel.theirCode.GetNotificationAddress();
        entry.push_back(Pair("notification_address",
                         CBitcoinAddress(notifAddr).ToString()));

        result.push_back(entry);
    }

    return result;
}

Value deriveaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "deriveaddress <payment_code> <index>\n"
            "Derive a specific address from a BIP47 payment code channel.\n"
            "Requires an established channel (notification sent or received).\n"
            "<index> is the address index (0, 1, 2, ...).\n"
            "Returns the derived address and public key.");

    EnsureWalletIsUnlocked();

    string strTheirCode = params[0].get_str();
    int nIndex = params[1].get_int();
    if (nIndex < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Index must be non-negative");

    CPaymentChannel channel;
    {
        LOCK(pwalletMain->cs_wallet);
        map<string, CPaymentChannel>::iterator it =
            pwalletMain->mapPaymentChannels.find(strTheirCode);
        if (it == pwalletMain->mapPaymentChannels.end())
        {

            CPaymentCode parsedCode;
            if (parsedCode.FromBase58(strTheirCode))
            {
                string strKey = parsedCode.ToBase58();
                it = pwalletMain->mapPaymentChannels.find(strKey);
            }

            if (it == pwalletMain->mapPaymentChannels.end())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                    "No payment channel found for this code. Send a notification first.");
        }
        channel = it->second;
    }

    CPubKey destPubKey;
    CKeyID  destKeyID;
    if (!channel.theirCode.DeriveReceiveAddress(channel.sharedSecret,
                                                 (uint32_t)nIndex,
                                                 destPubKey, destKeyID))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to derive address");

    {
        LOCK(pwalletMain->cs_wallet);
        map<string, CPaymentChannel>::iterator it =
            pwalletMain->mapPaymentChannels.find(strTheirCode);
        if (it != pwalletMain->mapPaymentChannels.end() &&
            (uint32_t)nIndex >= it->second.nNextSendIndex)
        {
            it->second.nNextSendIndex = (uint32_t)nIndex + 1;
            pwalletMain->AddPaymentChannel(strTheirCode, it->second);
        }
    }

    Object result;
    result.push_back(Pair("address", CBitcoinAddress(destKeyID).ToString()));
    result.push_back(Pair("index", nIndex));

    const vector<unsigned char>& vchPub = destPubKey.Raw();
    result.push_back(Pair("pubkey", HexStr(vchPub.begin(), vchPub.end())));

    return result;
}

Value scannotifications(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "scannotifications\n"
            "Scan the blockchain for BIP47 notification transactions.\n"
            "For each notification found, extract the sender's payment code\n"
            "and establish a payment channel for receiving payments.\n"
            "Returns the number of new payment codes discovered.\n"
            "Note: this can be slow on a long chain.");

    EnsureWalletIsUnlocked();

    CPaymentCode ourCode;
    CKey scanSecret, spendSecret;
    if (!GetOrCreatePaymentCode(ourCode, scanSecret, spendSecret))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get wallet payment code");

    CKeyID ourNotificationAddr = ourCode.GetNotificationAddress();

    int nFound = 0;

    CBlockIndex* pindex = pindexGenesisBlock;
    CTxDB txdb("r");

    while (pindex)
    {
        CBlock block;
        if (!block.ReadFromDisk(pindex, true))
        {
            pindex = pindex->pnext;
            continue;
        }

        BOOST_FOREACH(const CTransaction& tx, block.vtx)
        {

            bool fPaysToNotification = false;
            BOOST_FOREACH(const CTxOut& txout, tx.vout)
            {
                CTxDestination dest;
                if (ExtractDestination(txout.scriptPubKey, dest))
                {
                    CKeyID* pkeyid = boost::get<CKeyID>(&dest);
                    if (pkeyid && *pkeyid == ourNotificationAddr)
                    {
                        fPaysToNotification = true;
                        break;
                    }
                }
            }

            if (!fPaysToNotification)
                continue;

            BOOST_FOREACH(const CTxOut& txout, tx.vout)
            {
                vector<unsigned char> vchBlindedPayload;
                if (!ExtractNotificationPayload(txout.scriptPubKey, vchBlindedPayload))
                    continue;

                if (tx.vin.empty())
                    continue;

                const CTxIn& txin = tx.vin[0];

                CScript::const_iterator pc = txin.scriptSig.begin();
                opcodetype opcode;
                vector<unsigned char> vchSig;
                vector<unsigned char> vchInputPubKey;

                if (!txin.scriptSig.GetOp(pc, opcode, vchSig))
                    continue;

                if (!txin.scriptSig.GetOp(pc, opcode, vchInputPubKey))
                    continue;

                CPubKey inputPubKey(vchInputPubKey);
                if (!inputPubKey.IsValid() || !inputPubKey.IsCompressed())
                    continue;

                uint256 blindingSecret;
                if (!ComputeBIP47SharedSecret(scanSecret, inputPubKey, blindingSecret))
                    continue;

                vector<unsigned char> vchPayload;
                if (!UnblindPaymentCode(vchBlindedPayload, blindingSecret, vchPayload))
                    continue;

                CPaymentCode senderCode;
                if (!senderCode.Decode(vchPayload))
                    continue;

                if (!senderCode.IsValid())
                    continue;

                string strSenderCode = senderCode.ToBase58();
                {
                    LOCK(pwalletMain->cs_wallet);
                    if (pwalletMain->mapPaymentChannels.count(strSenderCode))
                    {

                        pwalletMain->mapPaymentChannels[strSenderCode].fNotificationRecv = true;
                        continue;
                    }
                }

                uint256 channelSecret;
                if (!ComputeBIP47SharedSecret(scanSecret, senderCode.spendPubKey, channelSecret))
                    continue;

                CPaymentChannel channel;
                channel.theirCode = senderCode;
                channel.sharedSecret = channelSecret;
                channel.fNotificationRecv = true;
                channel.nCreateTime = GetTime();

                pwalletMain->AddPaymentChannel(strSenderCode, channel);
                nFound++;

                printf("BIP47: Discovered payment code from notification TX %s\n",
                       tx.GetHash().GetHex().c_str());
            }
        }

        pindex = pindex->pnext;
    }

    return nFound;
}
