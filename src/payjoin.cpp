#include "payjoin.h"
#include "txdb.h"
#include "wallet.h"
#include "base58.h"
#include "init.h"
#include "util.h"
#include "script.h"
#include "net.h"

#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <algorithm>

using namespace std;

CPayJoinServer* pPayJoinServer = NULL;
std::vector<CPayJoinHistoryEntry> vPayJoinHistory;
CCriticalSection cs_payjoinHistory;

void RecordPayJoinHistory(const CPayJoinHistoryEntry& entry)
{
    LOCK(cs_payjoinHistory);
    vPayJoinHistory.push_back(entry);

    if (vPayJoinHistory.size() > 100)
        vPayJoinHistory.erase(vPayJoinHistory.begin());
}

std::vector<CPayJoinHistoryEntry> GetPayJoinHistory()
{
    LOCK(cs_payjoinHistory);
    return vPayJoinHistory;
}

CPayJoinServer::CPayJoinServer(CWallet* wallet)
    : pWallet(wallet), fRunning(false), nPort(0)
{
}

CPayJoinServer::~CPayJoinServer()
{
    Stop();
}

bool CPayJoinServer::IsRunning() const
{
    LOCK(cs_payjoin);
    return fRunning;
}

int CPayJoinServer::GetPort() const
{
    LOCK(cs_payjoin);
    return nPort;
}

int CPayJoinServer::GetProposalCount() const
{
    LOCK(cs_payjoin);
    return (int)mapProposals.size();
}

std::vector<CPayJoinProposal> CPayJoinServer::GetRecentProposals() const
{
    LOCK(cs_payjoin);
    std::vector<CPayJoinProposal> vResult;
    for (std::map<uint256, CPayJoinProposal>::const_iterator it = mapProposals.begin();
         it != mapProposals.end(); ++it)
    {
        vResult.push_back(it->second);
    }
    return vResult;
}

bool CPayJoinServer::ValidateOriginalTx(const CTransaction& tx,
                                         int64_t& nReceiverAmount,
                                         std::string& strError) const
{
    if (!pWallet)
    {
        strError = "Wallet not available";
        return false;
    }

    if (tx.vin.empty())
    {
        strError = "Transaction has no inputs";
        return false;
    }
    if (tx.vout.empty())
    {
        strError = "Transaction has no outputs";
        return false;
    }

    if (tx.IsCoinBase())
    {
        strError = "Transaction is a coinbase";
        return false;
    }
    if (tx.IsCoinStake())
    {
        strError = "Transaction is a coinstake";
        return false;
    }

    nReceiverAmount = 0;
    bool fFoundOurOutput = false;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        if (pWallet->IsMine(txout))
        {
            nReceiverAmount += txout.nValue;
            fFoundOurOutput = true;
        }
    }

    if (!fFoundOurOutput)
    {
        strError = "Transaction does not contain any output to our wallet";
        return false;
    }

    if (nReceiverAmount <= 0)
    {
        strError = "Receiver amount is zero or negative";
        return false;
    }

    int nOurInputs = 0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
        if (mi != pWallet->mapWallet.end())
        {
            const CWalletTx& wtxPrev = mi->second;
            if (txin.prevout.n < wtxPrev.vout.size() && pWallet->IsMine(wtxPrev.vout[txin.prevout.n]))
                nOurInputs++;
        }
    }

    if (nOurInputs == (int)tx.vin.size())
    {
        strError = "All inputs belong to our wallet — not a valid PayJoin request";
        return false;
    }

    return true;
}

int64_t CPayJoinServer::ScoreDenominationMatch(int64_t nUtxoValue,
                                                const std::vector<int64_t>& vSenderValues) const
{
    if (vSenderValues.empty())
        return nUtxoValue;

    int64_t nBestDiff = std::abs(nUtxoValue - vSenderValues[0]);
    for (size_t i = 1; i < vSenderValues.size(); i++)
    {
        int64_t nDiff = std::abs(nUtxoValue - vSenderValues[i]);
        if (nDiff < nBestDiff)
            nBestDiff = nDiff;
    }
    return nBestDiff;
}

bool CPayJoinServer::SelectReceiverInputs(
    const CTransaction& txOriginal,
    std::set<std::pair<const CWalletTx*, unsigned int> >& setCoins,
    int64_t& nValueIn)
{
    setCoins.clear();
    nValueIn = 0;

    if (!pWallet)
        return false;

    std::vector<int64_t> vSenderValues;
    BOOST_FOREACH(const CTxIn& txin, txOriginal.vin)
    {
        std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
        if (mi != pWallet->mapWallet.end())
        {
            const CWalletTx& wtxPrev = mi->second;
            if (txin.prevout.n < wtxPrev.vout.size())
                vSenderValues.push_back(wtxPrev.vout[txin.prevout.n].nValue);
        }
        else
        {

        }
    }

    if (vSenderValues.empty())
    {
        BOOST_FOREACH(const CTxOut& txout, txOriginal.vout)
        {
            if (txout.nValue > 0)
                vSenderValues.push_back(txout.nValue);
        }
    }

    std::vector<COutput> vCoins;
    pWallet->AvailableCoins(vCoins, true);

    std::vector<std::pair<int64_t, int> > vScored;
    for (int i = 0; i < (int)vCoins.size(); i++)
    {
        const COutput& out = vCoins[i];

        if (out.nDepth < PAYJOIN_MIN_CONFIRMATIONS)
            continue;

        bool fIsSenderInput = false;
        BOOST_FOREACH(const CTxIn& txin, txOriginal.vin)
        {
            if (txin.prevout.hash == out.tx->GetHash() &&
                txin.prevout.n == (unsigned int)out.i)
            {
                fIsSenderInput = true;
                break;
            }
        }
        if (fIsSenderInput)
            continue;

        int64_t nValue = out.tx->vout[out.i].nValue;
        int64_t nScore = ScoreDenominationMatch(nValue, vSenderValues);
        vScored.push_back(std::make_pair(nScore, i));
    }

    std::sort(vScored.begin(), vScored.end());

    int nSelected = 0;
    for (size_t i = 0; i < vScored.size() && nSelected < PAYJOIN_MAX_RECEIVER_INPUTS; i++)
    {
        const COutput& out = vCoins[vScored[i].second];
        int64_t nValue = out.tx->vout[out.i].nValue;

        int64_t nPaymentAmount = 0;
        BOOST_FOREACH(const CTxOut& txout, txOriginal.vout)
        {
            if (pWallet->IsMine(txout))
                nPaymentAmount += txout.nValue;
        }
        if (nValueIn + nValue > nPaymentAmount * 3 && nSelected > 0)
            break;

        setCoins.insert(std::make_pair(out.tx, out.i));
        nValueIn += nValue;
        nSelected++;
    }

    return nSelected > 0;
}

bool CPayJoinServer::AddReceiverInputs(CTransaction& tx,
                                        const CTransaction& txOriginal,
                                        std::string& strError)
{
    std::set<std::pair<const CWalletTx*, unsigned int> > setCoins;
    int64_t nValueIn = 0;

    if (!SelectReceiverInputs(txOriginal, setCoins, nValueIn))
    {
        strError = "No suitable receiver inputs available for PayJoin";
        return false;
    }

    if (nValueIn <= 0)
    {
        strError = "Selected receiver inputs have zero value";
        return false;
    }

    typedef std::pair<const CWalletTx*, unsigned int> CoinPair;
    BOOST_FOREACH(const CoinPair& coin, setCoins)
    {
        CTxIn newInput(coin.first->GetHash(), coin.second);

        if (tx.vin.empty())
            tx.vin.push_back(newInput);
        else
        {
            int nPos = GetRandInt(tx.vin.size() + 1);
            tx.vin.insert(tx.vin.begin() + nPos, newInput);
        }
    }

    printf("PayJoin: Added %d receiver inputs worth %s\n",
           (int)setCoins.size(), FormatMoney(nValueIn).c_str());

    return true;
}

bool CPayJoinServer::AdjustOutputs(CTransaction& tx,
                                    int64_t nReceiverAmount,
                                    int64_t nAddedInputValue,
                                    std::string& strError)
{
    if (!pWallet)
    {
        strError = "Wallet not available";
        return false;
    }

    if (nAddedInputValue <= 0)
    {
        strError = "No added input value to adjust";
        return false;
    }

    int64_t nAdditionalFee = MIN_TX_FEE;
    int64_t nReceiverChange = nAddedInputValue - nAdditionalFee;

    if (nReceiverChange <= 0)
    {

        printf("PayJoin: Receiver inputs consumed by fee contribution\n");
        return true;
    }

    CPubKey newKey;
    if (!pWallet->GetKeyFromPool(newKey, false))
    {
        strError = "Keypool ran out for receiver change address";
        return false;
    }

    CKeyID keyID = newKey.GetID();
    pWallet->SetAddressBookName(keyID, "payjoin-change");

    CScript scriptChange;
    scriptChange.SetDestination(keyID);

    CTxOut txoutChange(nReceiverChange, scriptChange);
    int nPos = GetRandInt(tx.vout.size() + 1);
    tx.vout.insert(tx.vout.begin() + nPos, txoutChange);

    printf("PayJoin: Added receiver change output of %s\n",
           FormatMoney(nReceiverChange).c_str());

    return true;
}

bool CPayJoinServer::CreateProposal(const CTransaction& txOriginal,
                                     CTransaction& txProposal,
                                     std::string& strError)
{
    LOCK(cs_payjoin);

    if (!pWallet)
    {
        strError = "Wallet not available";
        return false;
    }

    if (pWallet->IsLocked())
    {
        strError = "Wallet is locked — unlock with walletpassphrase first";
        return false;
    }

    int64_t nReceiverAmount = 0;
    if (!ValidateOriginalTx(txOriginal, nReceiverAmount, strError))
        return false;

    printf("PayJoin: Received valid proposal for %s MARYJ\n",
           FormatMoney(nReceiverAmount).c_str());

    txProposal = txOriginal;

    int64_t nOriginalInputValue = 0;
    BOOST_FOREACH(const CTxIn& txin, txOriginal.vin)
    {

        CTransaction txPrev;
        uint256 hashBlock = 0;
        if (GetTransaction(txin.prevout.hash, txPrev, hashBlock))
        {
            if (txin.prevout.n < txPrev.vout.size())
                nOriginalInputValue += txPrev.vout[txin.prevout.n].nValue;
        }
    }

    int nOriginalInputCount = (int)txOriginal.vin.size();

    if (!AddReceiverInputs(txProposal, txOriginal, strError))
        return false;

    int64_t nProposalInputValue = 0;
    int nReceiverInputCount = 0;
    BOOST_FOREACH(const CTxIn& txin, txProposal.vin)
    {

        std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
        if (mi != pWallet->mapWallet.end())
        {
            const CWalletTx& wtxPrev = mi->second;
            if (txin.prevout.n < wtxPrev.vout.size() && pWallet->IsMine(wtxPrev.vout[txin.prevout.n]))
            {
                nProposalInputValue += wtxPrev.vout[txin.prevout.n].nValue;
                nReceiverInputCount++;
            }
        }
    }

    BOOST_FOREACH(const CTxIn& txin, txProposal.vin)
    {
        CTransaction txPrev;
        uint256 hashBlock = 0;
        if (GetTransaction(txin.prevout.hash, txPrev, hashBlock))
        {
            if (txin.prevout.n < txPrev.vout.size())
            {

                std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
                bool fOurs = false;
                if (mi != pWallet->mapWallet.end())
                {
                    const CWalletTx& wtxPrev = mi->second;
                    if (txin.prevout.n < wtxPrev.vout.size() && pWallet->IsMine(wtxPrev.vout[txin.prevout.n]))
                        fOurs = true;
                }
                if (!fOurs)
                    nProposalInputValue += txPrev.vout[txin.prevout.n].nValue;
            }
        }
    }

    int64_t nReceiverAddedValue = nProposalInputValue - nOriginalInputValue;

    if (!AdjustOutputs(txProposal, nReceiverAmount, nReceiverAddedValue, strError))
        return false;

    {
        LOCK2(cs_main, pWallet->cs_wallet);
        for (unsigned int i = 0; i < txProposal.vin.size(); i++)
        {
            const CTxIn& txin = txProposal.vin[i];
            std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
            if (mi != pWallet->mapWallet.end())
            {
                const CWalletTx& wtxPrev = mi->second;
                if (txin.prevout.n < wtxPrev.vout.size() && pWallet->IsMine(wtxPrev.vout[txin.prevout.n]))
                {
                    if (!SignSignature(*pWallet, wtxPrev, txProposal, i))
                    {
                        strError = strprintf("Failed to sign receiver input %d", i);
                        return false;
                    }
                }
            }
        }
    }

    CPayJoinProposal proposal;
    proposal.txOriginal = txOriginal;
    proposal.txProposal = txProposal;
    proposal.nReceiverAmount = nReceiverAmount;
    proposal.nTimestamp = GetTime();
    proposal.fCompleted = false;
    mapProposals[txOriginal.GetHash()] = proposal;

    printf("PayJoin: Created proposal with %d sender + %d receiver inputs\n",
           nOriginalInputCount, nReceiverInputCount);

    return true;
}

bool CPayJoinServer::Start(int nListenPort, std::string& strError)
{
    LOCK(cs_payjoin);

    if (fRunning)
    {
        strError = "PayJoin server is already running";
        return false;
    }

    if (nListenPort <= 0 || nListenPort > 65535)
    {
        strError = strprintf("Invalid port: %d", nListenPort);
        return false;
    }

    nPort = nListenPort;
    fRunning = true;

    if (!NewThread(ThreadPayJoinServer, this))
    {
        fRunning = false;
        strError = "Failed to start PayJoin server thread";
        return false;
    }

    printf("PayJoin: Server started on port %d\n", nPort);
    return true;
}

void CPayJoinServer::Stop()
{
    LOCK(cs_payjoin);
    if (fRunning)
    {
        fRunning = false;
        nPort = 0;
        printf("PayJoin: Server stopped\n");
    }
}

void ThreadPayJoinServer(void* parg)
{
    CPayJoinServer* pServer = (CPayJoinServer*)parg;
    if (!pServer)
        return;

    RenameThread("MaryJaneCoin-payjoin");

    int nPort = pServer->GetPort();

    try
    {
        boost::asio::io_service ioService;
        boost::asio::ip::tcp::acceptor acceptor(ioService,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), nPort));

        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

        printf("PayJoin: HTTP server listening on port %d\n", nPort);

        while (pServer->IsRunning() && !fShutdown)
        {

            boost::asio::ip::tcp::socket socket(ioService);

            boost::system::error_code ec;
            acceptor.non_blocking(true);

            ec = boost::system::error_code();
            acceptor.accept(socket, ec);
            if (ec)
            {
                if (ec == boost::asio::error::would_block)
                {
                    MilliSleep(100);
                    continue;
                }
                printf("PayJoin: Accept error: %s\n", ec.message().c_str());
                MilliSleep(1000);
                continue;
            }

            boost::asio::streambuf request;
            boost::asio::read_until(socket, request, "\r\n\r\n", ec);
            if (ec)
            {
                printf("PayJoin: Read error: %s\n", ec.message().c_str());
                continue;
            }

            std::istream requestStream(&request);
            std::string strMethod, strPath, strVersion;
            requestStream >> strMethod >> strPath >> strVersion;

            std::string strHeaders;
            std::string strLine;
            int nContentLength = 0;
            while (std::getline(requestStream, strLine) && strLine != "\r")
            {
                strHeaders += strLine + "\n";

                if (strLine.find("Content-Length:") != std::string::npos ||
                    strLine.find("content-length:") != std::string::npos)
                {
                    std::string strLen = strLine.substr(strLine.find(":") + 1);
                    boost::trim(strLen);
                    nContentLength = atoi(strLen.c_str());
                }
            }

            std::string strBody;
            if (nContentLength > 0)
            {

                size_t nAlready = request.size();
                if (nAlready > 0)
                {
                    std::istream bodyStream(&request);
                    strBody.resize(nAlready);
                    bodyStream.read(&strBody[0], nAlready);
                }

                if ((int)strBody.size() < nContentLength)
                {
                    size_t nRemaining = nContentLength - strBody.size();
                    std::vector<char> buf(nRemaining);
                    boost::asio::read(socket, boost::asio::buffer(buf), ec);
                    if (!ec)
                        strBody.append(buf.begin(), buf.end());
                }
            }

            std::string strResponseBody;
            std::string strStatus = "200 OK";

            if (strMethod == "POST" && strPath == "/payjoin")
            {

                boost::trim(strBody);

                if (strBody.empty() || !IsHex(strBody))
                {
                    strStatus = "400 Bad Request";
                    strResponseBody = "Invalid hex-encoded transaction";
                }
                else
                {

                    std::vector<unsigned char> txData(ParseHex(strBody));
                    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
                    CTransaction txOriginal;

                    try
                    {
                        ssData >> txOriginal;
                    }
                    catch (std::exception& e)
                    {
                        strStatus = "400 Bad Request";
                        strResponseBody = "Failed to decode transaction";
                    }

                    if (strResponseBody.empty())
                    {

                        CTransaction txProposal;
                        std::string strError;

                        if (pServer->CreateProposal(txOriginal, txProposal, strError))
                        {

                            CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
                            ssTx << txProposal;
                            strResponseBody = HexStr(ssTx.begin(), ssTx.end());
                        }
                        else
                        {
                            strStatus = "422 Unprocessable Entity";
                            strResponseBody = strError;
                        }
                    }
                }
            }
            else
            {
                strStatus = "404 Not Found";
                strResponseBody = "PayJoin endpoint is POST /payjoin";
            }

            std::ostringstream response;
            response << "HTTP/1.1 " << strStatus << "\r\n"
                     << "Content-Type: text/plain\r\n"
                     << "Content-Length: " << strResponseBody.size() << "\r\n"
                     << "Connection: close\r\n"
                     << "\r\n"
                     << strResponseBody;

            std::string strResponse = response.str();
            boost::asio::write(socket, boost::asio::buffer(strResponse), ec);

            socket.close();
        }

        acceptor.close();
    }
    catch (std::exception& e)
    {
        printf("PayJoin: Server exception: %s\n", e.what());
    }

    printf("PayJoin: HTTP server thread exiting\n");
}

CPayJoinClient::CPayJoinClient(CWallet* wallet)
    : pWallet(wallet), nStatus(PAYJOIN_IDLE)
{
}

PayJoinStatus CPayJoinClient::GetStatus() const
{
    LOCK(cs_payjoin);
    return nStatus;
}

bool CPayJoinClient::CreateOriginalTx(const std::string& strAddress,
                                       int64_t nAmount,
                                       CTransaction& txOriginal,
                                       CReserveKey& reservekey,
                                       std::string& strError)
{
    CBitcoinAddress address(strAddress);
    if (!address.IsValid())
    {
        strError = "Invalid MaryJaneCoin address: " + strAddress;
        return false;
    }

    if (nAmount <= 0)
    {
        strError = "Invalid amount";
        return false;
    }

    if (nAmount > pWallet->GetBalance())
    {
        strError = "Insufficient funds";
        return false;
    }

    CScript scriptPubKey;
    scriptPubKey.SetDestination(address.Get());

    CWalletTx wtxNew;
    int64_t nFeeRequired = 0;
    bool fCreated = pWallet->CreateTransaction(scriptPubKey, nAmount, wtxNew, reservekey, nFeeRequired);

    if (!fCreated)
    {
        if (nAmount + nFeeRequired > pWallet->GetBalance())
            strError = strprintf("Insufficient funds including fee of %s",
                                  FormatMoney(nFeeRequired).c_str());
        else
            strError = "Transaction creation failed";
        return false;
    }

    txOriginal = (CTransaction)wtxNew;

    {
        LOCK(cs_payjoin);
        setOriginalInputs.clear();
        mapOriginalOutputs.clear();

        BOOST_FOREACH(const CTxIn& txin, txOriginal.vin)
            setOriginalInputs.insert(txin.prevout);

        BOOST_FOREACH(const CTxOut& txout, txOriginal.vout)
            mapOriginalOutputs[txout.scriptPubKey] = txout.nValue;
    }

    return true;
}

bool CPayJoinClient::SendProposal(const std::string& strEndpoint,
                                   const CTransaction& txOriginal,
                                   CTransaction& txProposal,
                                   std::string& strError)
{

    std::string strHost, strPath = "/payjoin";
    int nPort = 80;

    std::string strClean = strEndpoint;

    if (strClean.find("http://") == 0)
        strClean = strClean.substr(7);

    size_t nSlash = strClean.find('/');
    if (nSlash != std::string::npos)
    {
        strPath = strClean.substr(nSlash);
        strClean = strClean.substr(0, nSlash);
    }

    size_t nColon = strClean.find(':');
    if (nColon != std::string::npos)
    {
        strHost = strClean.substr(0, nColon);
        nPort = atoi(strClean.substr(nColon + 1).c_str());
    }
    else
    {
        strHost = strClean;
    }

    if (strHost.empty() || nPort <= 0)
    {
        strError = "Invalid endpoint: " + strEndpoint;
        return false;
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << txOriginal;
    std::string strHexTx = HexStr(ssTx.begin(), ssTx.end());

    std::ostringstream httpRequest;
    httpRequest << "POST " << strPath << " HTTP/1.1\r\n"
                << "Host: " << strHost << "\r\n"
                << "Content-Type: text/plain\r\n"
                << "Content-Length: " << strHexTx.size() << "\r\n"
                << "Connection: close\r\n"
                << "\r\n"
                << strHexTx;

    try
    {
        boost::asio::io_service ioService;
        boost::asio::ip::tcp::resolver resolver(ioService);
        boost::asio::ip::tcp::resolver::query query(strHost, strprintf("%d", nPort));
        boost::asio::ip::tcp::resolver::iterator endpoint = resolver.resolve(query);

        boost::asio::ip::tcp::socket socket(ioService);
        boost::asio::connect(socket, endpoint);

        std::string strReq = httpRequest.str();
        boost::asio::write(socket, boost::asio::buffer(strReq));

        boost::asio::streambuf response;
        boost::system::error_code ec;
        boost::asio::read_until(socket, response, "\r\n", ec);

        std::istream responseStream(&response);
        std::string strHttpVersion;
        unsigned int nStatusCode;
        std::string strStatusMsg;
        responseStream >> strHttpVersion >> nStatusCode;
        std::getline(responseStream, strStatusMsg);

        if (nStatusCode != 200)
        {

            boost::asio::read_until(socket, response, "\r\n\r\n", ec);
            std::string strLine;
            while (std::getline(responseStream, strLine) && strLine != "\r") {}

            std::string strBody;
            boost::asio::read(socket, response, ec);
            std::istream bodyStream(&response);
            std::ostringstream bodyOss;
            bodyOss << bodyStream.rdbuf();
            strBody = bodyOss.str();

            strError = strprintf("PayJoin server returned HTTP %d: %s",
                                  nStatusCode, strBody.c_str());
            return false;
        }

        boost::asio::read_until(socket, response, "\r\n\r\n", ec);
        std::string strLine;
        int nContentLength = 0;
        while (std::getline(responseStream, strLine) && strLine != "\r")
        {
            if (strLine.find("Content-Length:") != std::string::npos ||
                strLine.find("content-length:") != std::string::npos)
            {
                std::string strLen = strLine.substr(strLine.find(":") + 1);
                boost::trim(strLen);
                nContentLength = atoi(strLen.c_str());
            }
        }

        std::string strResponseBody;
        {
            std::ostringstream bodyOss;
            if (response.size() > 0)
                bodyOss << &response;

            boost::asio::read(socket, response, ec);
            if (response.size() > 0)
                bodyOss << &response;
            strResponseBody = bodyOss.str();
        }

        boost::trim(strResponseBody);

        if (strResponseBody.empty() || !IsHex(strResponseBody))
        {
            strError = "Receiver returned invalid hex response";
            return false;
        }

        std::vector<unsigned char> proposalData(ParseHex(strResponseBody));
        CDataStream ssProposal(proposalData, SER_NETWORK, PROTOCOL_VERSION);

        try
        {
            ssProposal >> txProposal;
        }
        catch (std::exception& e)
        {
            strError = "Failed to decode proposal transaction";
            return false;
        }

        socket.close();
    }
    catch (std::exception& e)
    {
        strError = strprintf("Connection failed: %s", e.what());
        return false;
    }

    return true;
}

bool CPayJoinClient::ValidateProposal(const CTransaction& txOriginal,
                                       const CTransaction& txProposal,
                                       const std::string& strReceiverAddress,
                                       int64_t nAmount,
                                       std::string& strError) const
{

    std::set<COutPoint> setProposalInputs;
    BOOST_FOREACH(const CTxIn& txin, txProposal.vin)
        setProposalInputs.insert(txin.prevout);

    BOOST_FOREACH(const CTxIn& txin, txOriginal.vin)
    {
        if (setProposalInputs.find(txin.prevout) == setProposalInputs.end())
        {
            strError = strprintf("Proposal removed our input %s:%d — REJECTED",
                                  txin.prevout.hash.GetHex().c_str(), txin.prevout.n);
            return false;
        }
    }

    if (txProposal.vin.size() <= txOriginal.vin.size())
    {
        strError = "Proposal has no additional inputs from receiver";
        return false;
    }

    CBitcoinAddress receiverAddr(strReceiverAddress);
    if (!receiverAddr.IsValid())
    {
        strError = "Invalid receiver address for validation";
        return false;
    }

    CScript scriptReceiver;
    scriptReceiver.SetDestination(receiverAddr.Get());

    int64_t nReceiverTotal = 0;
    BOOST_FOREACH(const CTxOut& txout, txProposal.vout)
    {
        if (txout.scriptPubKey == scriptReceiver)
            nReceiverTotal += txout.nValue;
    }

    if (nReceiverTotal < nAmount)
    {
        strError = strprintf("Receiver output reduced from %s to %s — REJECTED",
                              FormatMoney(nAmount).c_str(),
                              FormatMoney(nReceiverTotal).c_str());
        return false;
    }

    int64_t nOrigTotalIn = 0, nOrigTotalOut = 0;
    int64_t nPropTotalOut = 0;

    BOOST_FOREACH(const CTxOut& txout, txOriginal.vout)
        nOrigTotalOut += txout.nValue;

    BOOST_FOREACH(const CTxOut& txout, txProposal.vout)
        nPropTotalOut += txout.nValue;

    BOOST_FOREACH(const CTxIn& txin, txOriginal.vin)
    {

        std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
        if (mi != pWallet->mapWallet.end())
        {
            const CWalletTx& wtxPrev = mi->second;
            if (txin.prevout.n < wtxPrev.vout.size())
                nOrigTotalIn += wtxPrev.vout[txin.prevout.n].nValue;
        }
        else
        {

            CTransaction txPrev;
            uint256 hashBlock = 0;
            if (GetTransaction(txin.prevout.hash, txPrev, hashBlock))
            {
                if (txin.prevout.n < txPrev.vout.size())
                    nOrigTotalIn += txPrev.vout[txin.prevout.n].nValue;
            }
        }
    }

    int64_t nOrigFee = nOrigTotalIn - nOrigTotalOut;

    int64_t nOurOrigChange = 0;
    int64_t nOurPropChange = 0;

    BOOST_FOREACH(const CTxOut& txout, txOriginal.vout)
    {
        if (pWallet->IsMine(txout) && txout.scriptPubKey != scriptReceiver)
            nOurOrigChange += txout.nValue;
    }

    BOOST_FOREACH(const CTxOut& txout, txProposal.vout)
    {
        if (pWallet->IsMine(txout) && txout.scriptPubKey != scriptReceiver)
            nOurPropChange += txout.nValue;
    }

    int64_t nChangeReduction = nOurOrigChange - nOurPropChange;
    if (nChangeReduction > PAYJOIN_MAX_FEE_INCREASE)
    {
        strError = strprintf("Change reduced by %s, exceeds max allowed %s — REJECTED",
                              FormatMoney(nChangeReduction).c_str(),
                              FormatMoney(PAYJOIN_MAX_FEE_INCREASE).c_str());
        return false;
    }

    int nNewOutputs = 0;
    BOOST_FOREACH(const CTxOut& txout, txProposal.vout)
    {
        bool fFoundInOriginal = false;
        BOOST_FOREACH(const CTxOut& origOut, txOriginal.vout)
        {
            if (txout.scriptPubKey == origOut.scriptPubKey)
            {
                fFoundInOriginal = true;
                break;
            }
        }
        if (!fFoundInOriginal)
            nNewOutputs++;
    }

    if (nNewOutputs > PAYJOIN_MAX_RECEIVER_INPUTS + 1)
    {
        strError = strprintf("Proposal adds %d new outputs — suspiciously many", nNewOutputs);
        return false;
    }

    printf("PayJoin: Proposal validated — %d sender + %d new inputs, fee increase: %s\n",
           (int)txOriginal.vin.size(),
           (int)(txProposal.vin.size() - txOriginal.vin.size()),
           FormatMoney(nChangeReduction).c_str());

    return true;
}

bool CPayJoinClient::SignAndBroadcast(CTransaction& txProposal,
                                       const CTransaction& txOriginal,
                                       CReserveKey& reservekey,
                                       CPayJoinResult& result,
                                       std::string& strError)
{
    {
        LOCK2(cs_main, pWallet->cs_wallet);

        for (unsigned int i = 0; i < txProposal.vin.size(); i++)
        {
            const CTxIn& txin = txProposal.vin[i];

            std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
            if (mi != pWallet->mapWallet.end())
            {
                const CWalletTx& wtxPrev = mi->second;
                if (txin.prevout.n < wtxPrev.vout.size() && pWallet->IsMine(wtxPrev.vout[txin.prevout.n]))
                {
                    if (!SignSignature(*pWallet, wtxPrev, txProposal, i))
                    {
                        strError = strprintf("Failed to sign input %d", i);
                        return false;
                    }
                }
            }
        }

        unsigned int nBytes = ::GetSerializeSize(txProposal, SER_NETWORK, PROTOCOL_VERSION);
        if (nBytes >= MAX_BLOCK_SIZE_GEN / 5)
        {
            strError = "PayJoin transaction too large";
            return false;
        }

        CTxDB txdb("r");
        if (!txProposal.AcceptToMemoryPool(txdb))
        {
            strError = "PayJoin transaction rejected by mempool";
            return false;
        }

        SyncWithWallets(txProposal, NULL, true);
        RelayTransaction(txProposal, txProposal.GetHash());
    }

    result.txHash = txProposal.GetHash();
    result.nOutputs = (int)txProposal.vout.size();
    result.fSuccess = true;
    result.fPayJoinUsed = true;

    result.nTotalInputs = (int)txProposal.vin.size();
    result.nSenderInputs = (int)txOriginal.vin.size();
    result.nReceiverInputs = result.nTotalInputs - result.nSenderInputs;

    int64_t nTotalIn = 0;
    BOOST_FOREACH(const CTxIn& txin, txProposal.vin)
    {
        std::map<uint256, CWalletTx>::const_iterator mi = pWallet->mapWallet.find(txin.prevout.hash);
        if (mi != pWallet->mapWallet.end())
        {
            const CWalletTx& wtxPrev = mi->second;
            if (txin.prevout.n < wtxPrev.vout.size())
                nTotalIn += wtxPrev.vout[txin.prevout.n].nValue;
        }
        else
        {
            CTransaction txPrev;
            uint256 hashBlock = 0;
            if (GetTransaction(txin.prevout.hash, txPrev, hashBlock))
            {
                if (txin.prevout.n < txPrev.vout.size())
                    nTotalIn += txPrev.vout[txin.prevout.n].nValue;
            }
        }
    }
    int64_t nTotalOut = 0;
    BOOST_FOREACH(const CTxOut& txout, txProposal.vout)
        nTotalOut += txout.nValue;
    result.nFeePaid = nTotalIn - nTotalOut;

    return true;
}

CPayJoinResult CPayJoinClient::SendPayJoin(const std::string& strAddress,
                                             int64_t nAmount,
                                             const std::string& strEndpoint)
{
    LOCK(cs_payjoin);
    CPayJoinResult result;
    result.nAmount = nAmount;
    std::string strError;

    if (!pWallet)
    {
        result.strError = "Wallet not available";
        nStatus = PAYJOIN_ERROR;
        return result;
    }

    if (pWallet->IsLocked())
    {
        result.strError = "Wallet is locked. Unlock with walletpassphrase first.";
        nStatus = PAYJOIN_ERROR;
        return result;
    }

    nStatus = PAYJOIN_CREATING;
    CReserveKey reservekey(pWallet);
    CTransaction txOriginal;

    if (!CreateOriginalTx(strAddress, nAmount, txOriginal, reservekey, strError))
    {
        result.strError = strError;
        nStatus = PAYJOIN_ERROR;
        return result;
    }

    printf("PayJoin: Created original TX %s with %d inputs\n",
           txOriginal.GetHash().GetHex().c_str(), (int)txOriginal.vin.size());

    nStatus = PAYJOIN_WAITING;
    CTransaction txProposal;

    if (!SendProposal(strEndpoint, txOriginal, txProposal, strError))
    {

        printf("PayJoin: Proposal failed (%s), falling back to normal TX\n", strError.c_str());

        CWalletTx wtxFallback(pWallet, txOriginal);
        wtxFallback.fFromMe = true;
        if (!pWallet->CommitTransaction(wtxFallback, reservekey))
        {
            result.strError = "PayJoin failed and fallback broadcast also failed";
            nStatus = PAYJOIN_ERROR;
            return result;
        }

        result.txHash = txOriginal.GetHash();
        result.nSenderInputs = (int)txOriginal.vin.size();
        result.nReceiverInputs = 0;
        result.nTotalInputs = result.nSenderInputs;
        result.nOutputs = (int)txOriginal.vout.size();
        result.fSuccess = true;
        result.fPayJoinUsed = false;
        result.strError = "PayJoin unavailable: " + strError + ". Sent as normal TX.";

        nStatus = PAYJOIN_COMPLETE;
        return result;
    }

    nStatus = PAYJOIN_VALIDATING;
    if (!ValidateProposal(txOriginal, txProposal, strAddress, nAmount, strError))
    {

        result.strError = "SECURITY: Proposal rejected — " + strError;
        nStatus = PAYJOIN_ERROR;
        reservekey.ReturnKey();
        return result;
    }

    nStatus = PAYJOIN_SIGNING;
    if (!SignAndBroadcast(txProposal, txOriginal, reservekey, result, strError))
    {
        result.strError = strError;
        nStatus = PAYJOIN_ERROR;
        return result;
    }

    CPayJoinHistoryEntry histEntry;
    histEntry.txHash = result.txHash;
    histEntry.nTimestamp = GetTime();
    histEntry.nAmount = nAmount;
    histEntry.fSender = true;
    histEntry.nSenderInputs = result.nSenderInputs;
    histEntry.nReceiverInputs = result.nReceiverInputs;
    histEntry.strCounterparty = strAddress;
    RecordPayJoinHistory(histEntry);

    nStatus = PAYJOIN_COMPLETE;
    printf("PayJoin: SUCCESS — TX %s broadcast with %d+%d inputs\n",
           result.txHash.GetHex().c_str(), result.nSenderInputs, result.nReceiverInputs);

    return result;
}
