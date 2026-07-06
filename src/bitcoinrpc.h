#ifndef _BITCOINRPC_H_
#define _BITCOINRPC_H_ 1

#include <string>
#include <list>
#include <map>

class CBlockIndex;

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#include "util.h"
#include "checkpoints.h"

enum HTTPStatusCode
{
    HTTP_OK                    = 200,
    HTTP_BAD_REQUEST           = 400,
    HTTP_UNAUTHORIZED          = 401,
    HTTP_FORBIDDEN             = 403,
    HTTP_NOT_FOUND             = 404,
    HTTP_INTERNAL_SERVER_ERROR = 500,
};

enum RPCErrorCode
{

    RPC_INVALID_REQUEST  = -32600,
    RPC_METHOD_NOT_FOUND = -32601,
    RPC_INVALID_PARAMS   = -32602,
    RPC_INTERNAL_ERROR   = -32603,
    RPC_PARSE_ERROR      = -32700,

    RPC_MISC_ERROR                  = -1,
    RPC_FORBIDDEN_BY_SAFE_MODE      = -2,
    RPC_TYPE_ERROR                  = -3,
    RPC_INVALID_ADDRESS_OR_KEY      = -5,
    RPC_OUT_OF_MEMORY               = -7,
    RPC_INVALID_PARAMETER           = -8,
    RPC_DATABASE_ERROR              = -20,
    RPC_DESERIALIZATION_ERROR       = -22,

    RPC_CLIENT_NOT_CONNECTED        = -9,
    RPC_CLIENT_IN_INITIAL_DOWNLOAD  = -10,

    RPC_WALLET_ERROR                = -4,
    RPC_WALLET_INSUFFICIENT_FUNDS   = -6,
    RPC_WALLET_INVALID_ACCOUNT_NAME = -11,
    RPC_WALLET_KEYPOOL_RAN_OUT      = -12,
    RPC_WALLET_UNLOCK_NEEDED        = -13,
    RPC_WALLET_PASSPHRASE_INCORRECT = -14,
    RPC_WALLET_WRONG_ENC_STATE      = -15,
    RPC_WALLET_ENCRYPTION_FAILED    = -16,
    RPC_WALLET_ALREADY_UNLOCKED     = -17,
};

json_spirit::Object JSONRPCError(int code, const std::string& message);

void ThreadRPCServer(void* parg);
int CommandLineRPC(int argc, char *argv[]);

json_spirit::Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams);

void RPCTypeCheck(const json_spirit::Array& params,
                  const std::list<json_spirit::Value_type>& typesExpected, bool fAllowNull=false);

void RPCTypeCheck(const json_spirit::Object& o,
                  const std::map<std::string, json_spirit::Value_type>& typesExpected, bool fAllowNull=false);

typedef json_spirit::Value(*rpcfn_type)(const json_spirit::Array& params, bool fHelp);

class CRPCCommand
{
public:
    std::string name;
    rpcfn_type actor;
    bool okSafeMode;
    bool unlocked;
};

class CRPCTable
{
private:
    std::map<std::string, const CRPCCommand*> mapCommands;
public:
    CRPCTable();
    const CRPCCommand* operator[](std::string name) const;
    std::string help(std::string name) const;

    json_spirit::Value execute(const std::string &method, const json_spirit::Array &params) const;
};

extern const CRPCTable tableRPC;

extern int64_t nWalletUnlockTime;
extern int64_t AmountFromValue(const json_spirit::Value& value);
extern json_spirit::Value ValueFromAmount(int64_t amount);
extern double GetDifficulty(const CBlockIndex* blockindex = NULL);

extern double GetPoWMHashPS();
extern double GetPoSKernelPS();

extern std::string HexBits(unsigned int nBits);
extern std::string HelpRequiringPassphrase();
extern void EnsureWalletIsUnlocked();

extern uint256 ParseHashV(const json_spirit::Value& v, std::string strName);
extern uint256 ParseHashO(const json_spirit::Object& o, std::string strKey);
extern std::vector<unsigned char> ParseHexV(const json_spirit::Value& v, std::string strName);
extern std::vector<unsigned char> ParseHexO(const json_spirit::Object& o, std::string strKey);

extern json_spirit::Value getconnectioncount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getpeerinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value addnode(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaddednodeinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value dumpwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value importwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value dumpprivkey(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value importprivkey(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value sendalert(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getsubsidy(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getmininginfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getstakinginfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getwork(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getworkex(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblocktemplate(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitblock(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getnewaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaccountaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaddressesbyaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendtoaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value signmessage(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value verifymessage(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getreceivedbyaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getreceivedbyaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getbalance(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value movecmd(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendfrom(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendmany(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value addmultisigaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value addredeemscript(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listreceivedbyaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listreceivedbyaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listtransactions(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listaddressgroupings(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listaccounts(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listsinceblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gettransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getstaketx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value deleteaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value backupwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value keypoolrefill(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletpassphrase(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletpassphrasechange(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletlock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value encryptwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value validateaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value reservebalance(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value checkwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value repairwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value resendtx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value rescanfromblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value makekeypair(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value validatepubkey(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getnewpubkey(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value stakeforcharity(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value setstakesplitthreshold(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getstakesplitthreshold(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value cclistcoins(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value ccselect(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value cclistselected(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value ccreturnchange(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value cccustomchange(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value ccreset(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value ccsend(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getweight(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getconfs(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value multisend(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value hashsettings(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value bridgetosol(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getpaymentcode(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendtonotify(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listpaymentcodes(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value deriveaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value scannotifications(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getrawtransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value listunspent(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value createrawtransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value decoderawtransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value decodescript(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value signrawtransaction(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendrawtransaction(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getbestblockhash(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockcount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getdifficulty(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value settxfee(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getrawmempool(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockhash(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockbynumber(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getcheckpoint(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value createbootstrap(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value reorganizetoheight(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value moneysupply(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getmoneysupply(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value coinjoin(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value coinjoinauto(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value coinjoininfo(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getnewstealthaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value liststealthaddresses(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value sendtostealthaddress(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value scanstealthpayments(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value pegin(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value pegout(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getpoolbalances(const json_spirit::Array& params, bool fHelp);

#endif
