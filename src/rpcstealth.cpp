#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "main.h"
#include "init.h"
#include "base58.h"
#include "stealth.h"
#include "script.h"
#include "txdb.h"

#include <boost/foreach.hpp>

using namespace json_spirit;
using namespace std;

extern CWallet* pwalletMain;

Value getnewstealthaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewstealthaddress [label]\n"
            "Generate a new stealth address.\n"
            "Anyone who knows this address can send you a private payment.\n"
            "Each payment goes to a unique one-time address on the blockchain.\n"
            "Returns the encoded stealth address string.");

    EnsureWalletIsUnlocked();

    string strLabel;
    if (params.size() > 0)
        strLabel = params[0].get_str();

    CStealthAddress sxAddr;
    CKey scanSecret;
    CKey spendSecret;
    if (!GenerateStealthAddress(sxAddr, scanSecret, spendSecret))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to generate stealth address keys");

    sxAddr.label = strLabel;

    if (!pwalletMain->AddKey(scanSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to store scan key in wallet");
    if (!pwalletMain->AddKey(spendSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to store spend key in wallet");

    if (!pwalletMain->AddStealthAddress(sxAddr))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to store stealth address in wallet");

    return sxAddr.Encoded();
}

Value liststealthaddresses(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "liststealthaddresses\n"
            "List all stealth addresses in the wallet.\n"
            "Returns an array of objects with keys: label, address, scan_pubkey, spend_pubkey.");

    vector<CStealthAddress> vStealth;
    pwalletMain->GetStealthAddresses(vStealth);

    Array result;
    BOOST_FOREACH(const CStealthAddress& sxAddr, vStealth)
    {
        Object entry;
        entry.push_back(Pair("label",        sxAddr.label));
        entry.push_back(Pair("address",      sxAddr.Encoded()));

        const vector<unsigned char>& vchScan  = sxAddr.scanPubKey.Raw();
        const vector<unsigned char>& vchSpend = sxAddr.spendPubKey.Raw();
        entry.push_back(Pair("scan_pubkey",  HexStr(vchScan.begin(),  vchScan.end())));
        entry.push_back(Pair("spend_pubkey", HexStr(vchSpend.begin(), vchSpend.end())));

        result.push_back(entry);
    }
    return result;
}

Value sendtostealthaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "sendtostealthaddress <stealth_address> <amount>\n"
            "Send <amount> coins privately to a stealth address.\n"
            "<amount> is a real number and is rounded to the nearest 0.000001.\n"
            "Returns the transaction id."
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();

    string strStealthAddr = params[0].get_str();
    CStealthAddress sxAddr;
    if (!sxAddr.SetEncoded(strStealthAddr))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid stealth address");

    int64_t nAmount = AmountFromValue(params[1]);

    CStealthPayment payment;
    if (!PrepareStealthPayment(sxAddr, payment))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to prepare stealth payment");

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
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }

    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value scanstealthpayments(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "scanstealthpayments\n"
            "Scan the entire blockchain for stealth payments destined for our stealth addresses.\n"
            "For each found payment, the one-time spend key is imported into the wallet.\n"
            "Returns the number of new stealth payments found.\n"
            "Note: this can be slow on a long chain.");

    EnsureWalletIsUnlocked();

    vector<CStealthAddress> vStealth;
    pwalletMain->GetStealthAddresses(vStealth);

    if (vStealth.empty())
        return 0;

    struct StealthEntry
    {
        CKey      scanSecret;
        CPubKey   spendPubKey;
        CKey      spendSecret;
        bool      hasSpendSecret;
    };
    vector<StealthEntry> vEntries;

    {
        LOCK(pwalletMain->cs_wallet);
        BOOST_FOREACH(const CStealthAddress& sxAddr, vStealth)
        {
            StealthEntry e;
            CKeyID scanKeyID  = sxAddr.scanPubKey.GetID();
            CKeyID spendKeyID = sxAddr.spendPubKey.GetID();

            if (!pwalletMain->GetKey(scanKeyID, e.scanSecret))
                continue;

            e.spendPubKey = sxAddr.spendPubKey;
            e.hasSpendSecret = pwalletMain->GetKey(spendKeyID, e.spendSecret);
            vEntries.push_back(e);
        }
    }

    if (vEntries.empty())
        return 0;

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

            BOOST_FOREACH(const CTxOut& txout, tx.vout)
            {
                const CScript& script = txout.scriptPubKey;

                if (script.size() != 35)
                    continue;
                if (script[0] != OP_RETURN)
                    continue;
                if (script[1] != 0x21)
                    continue;

                vector<unsigned char> vchEphemeral(script.begin() + 2, script.end());
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

                    bool fFound = false;
                    BOOST_FOREACH(const CTxOut& txout2, tx.vout)
                    {
                        CTxDestination dest;
                        if (ExtractDestination(txout2.scriptPubKey, dest))
                        {
                            CKeyID* pkeyid = boost::get<CKeyID>(&dest);
                            if (pkeyid && *pkeyid == destKeyID)
                            {
                                fFound = true;
                                break;
                            }
                        }
                    }

                    if (!fFound)
                        continue;

                    {
                        LOCK(pwalletMain->cs_wallet);
                        if (pwalletMain->HaveKey(destKeyID))
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

                    if (pwalletMain->AddKey(destKey))
                        nFound++;
                }
            }
        }

        pindex = pindex->pnext;
    }

    return nFound;
}
