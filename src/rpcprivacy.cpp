#include "bitcoinrpc.h"
#include "main.h"
#include "wallet.h"
#include "dandelion.h"
#include "torcontrol.h"
#include "i2p.h"
#include "payjoin.h"
#include "util.h"
#include "net.h"

extern CWallet* pwalletMain;

using namespace json_spirit;
using namespace std;

Value getprivacyinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getprivacyinfo\n"
            "Returns network privacy status for this node.\n"
            "\nResult:\n"
            "{\n"
            "  \"dandelion\": {                (object) Dandelion++ relay privacy\n"
            "    \"active\": true|false,       (boolean) is Dandelion++ routing active\n"
            "    \"epoch_start\": nnn,         (numeric) current epoch start time\n"
            "    \"stem_peers\": n,            (numeric) number of stem relay peers\n"
            "    \"stem_txs\": n,              (numeric) transactions in stem phase\n"
            "    \"fluffed_txs\": n            (numeric) transactions already fluffed\n"
            "  },\n"
            "  \"tor\": {                      (object) Tor integration status\n"
            "    \"connected\": true|false,    (boolean) connected to Tor control port\n"
            "    \"state\": \"...\",           (string) control connection state\n"
            "    \"hidden_service\": \"...\",  (string) .onion address if created\n"
            "    \"proxy_configured\": true|false, (boolean) is SOCKS proxy configured for Tor\n"
            "    \"toronly\": true|false       (boolean) is -toronly mode active\n"
            "  },\n"
            "  \"i2p\": {                      (object) I2P SAM integration status\n"
            "    \"connected\": true|false,    (boolean) connected to I2P SAM bridge\n"
            "    \"state\": \"...\",           (string) SAM session state\n"
            "    \"address\": \"...\",         (string) our .b32.i2p address\n"
            "    \"accept_incoming\": true|false (boolean) accepting inbound I2P connections\n"
            "  },\n"
            "  \"privacy_score\": n            (numeric) overall privacy score 0-100\n"
            "}\n"
        );

    Object result;

    {
        Object dandelion;
        int nStemPeers = g_dandelion.GetStemPeerCount();
        bool fActive = (nStemPeers > 0);

        dandelion.push_back(Pair("active", fActive));
        dandelion.push_back(Pair("epoch_start", (boost::int64_t)g_dandelion.GetEpochStart()));
        dandelion.push_back(Pair("stem_peers", nStemPeers));
        dandelion.push_back(Pair("stem_txs", g_dandelion.GetStemTxCount()));
        dandelion.push_back(Pair("fluffed_txs", g_dandelion.GetFluffedTxCount()));

        result.push_back(Pair("dandelion", dandelion));
    }

    {
        Object tor;
        TorControlState torState = g_torControl.GetState();
        bool fTorConnected = (torState >= TOR_AUTHENTICATED);
        std::string strOnion = g_torControl.GetOnionAddress();
        bool fTorOnly = GetBoolArg("-toronly", false);

        proxyType proxyInfo;
        bool fProxyConfigured = GetProxy(NET_TOR, proxyInfo);

        tor.push_back(Pair("connected", fTorConnected));

        std::string strState;
        switch (torState) {
            case TOR_DISCONNECTED:    strState = "disconnected"; break;
            case TOR_CONNECTING:      strState = "connecting"; break;
            case TOR_AUTHENTICATING:  strState = "authenticating"; break;
            case TOR_AUTHENTICATED:   strState = "authenticated"; break;
            case TOR_SERVICE_CREATED: strState = "hidden_service_active"; break;
            case TOR_ERROR:           strState = "error"; break;
            default:                  strState = "unknown"; break;
        }
        tor.push_back(Pair("state", strState));
        tor.push_back(Pair("hidden_service", strOnion.empty() ? "" : strOnion));
        tor.push_back(Pair("proxy_configured", fProxyConfigured));
        tor.push_back(Pair("toronly", fTorOnly));

        result.push_back(Pair("tor", tor));
    }

    {
        Object i2p;
        I2PSessionState i2pState = g_i2pSession.GetState();
        bool fI2PConnected = (i2pState == I2P_SESSION_CREATED);
        std::string strB32 = g_i2pSession.GetMyDestinationB32();
        bool fAcceptIncoming = GetBoolArg("-i2pacceptincoming", true);

        i2p.push_back(Pair("connected", fI2PConnected));

        std::string strState;
        switch (i2pState) {
            case I2P_DISCONNECTED:    strState = "disconnected"; break;
            case I2P_HELLO_SENT:      strState = "hello_sent"; break;
            case I2P_HELLO_OK:        strState = "hello_ok"; break;
            case I2P_SESSION_CREATED: strState = "session_active"; break;
            case I2P_ACCEPTING:       strState = "accepting"; break;
            case I2P_CONNECTED:       strState = "connected"; break;
            case I2P_ERROR:           strState = "error"; break;
            default:                  strState = "unknown"; break;
        }
        i2p.push_back(Pair("state", strState));
        i2p.push_back(Pair("address", strB32.empty() ? "" : strB32));
        i2p.push_back(Pair("accept_incoming", fAcceptIncoming && fI2PConnected));

        result.push_back(Pair("i2p", i2p));
    }

    {
        Object payjoinObj;
        bool fServerRunning = (pPayJoinServer && pPayJoinServer->IsRunning());
        payjoinObj.push_back(Pair("server_running", fServerRunning));
        payjoinObj.push_back(Pair("server_port", fServerRunning ? pPayJoinServer->GetPort() : 0));
        payjoinObj.push_back(Pair("proposals", fServerRunning ? pPayJoinServer->GetProposalCount() : 0));

        vector<CPayJoinHistoryEntry> vHistory = GetPayJoinHistory();
        payjoinObj.push_back(Pair("history_count", (int)vHistory.size()));

        result.push_back(Pair("payjoin", payjoinObj));
    }

    {
        int nScore = 0;

        if (g_dandelion.GetStemPeerCount() > 0)
            nScore += 20;

        proxyType proxyInfo;
        if (GetProxy(NET_TOR, proxyInfo))
            nScore += 15;
        if (g_torControl.GetState() == TOR_SERVICE_CREATED)
            nScore += 15;
        if (GetBoolArg("-toronly", false))
            nScore += 10;

        if (g_i2pSession.GetState() == I2P_SESSION_CREATED)
            nScore += 15;
        if (g_i2pSession.GetState() == I2P_SESSION_CREATED &&
            GetBoolArg("-i2pacceptincoming", true))
            nScore += 10;

        if (pPayJoinServer && pPayJoinServer->IsRunning())
            nScore += 15;

        result.push_back(Pair("privacy_score", nScore));
    }

    {
        Object pools;
        bool fTwoPoolActive = (nBestHeight >= TWO_POOL_ACTIVATION_HEIGHT);
        pools.push_back(Pair("active", fTwoPoolActive));
        pools.push_back(Pair("activation_height", TWO_POOL_ACTIVATION_HEIGHT));

        if (fTwoPoolActive && pwalletMain)
        {
            pools.push_back(Pair("transparent_balance", ValueFromAmount(pwalletMain->GetTransparentBalance())));
            pools.push_back(Pair("shielded_balance", ValueFromAmount(pwalletMain->GetShieldedBalance())));
        }

        result.push_back(Pair("two_pool", pools));
    }

    return result;
}
