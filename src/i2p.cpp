#include "i2p.h"
#include "util.h"
#include "net.h"
#include "hash.h"

#include <cstdio>
#include <cstring>
#include <sstream>

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

CI2PSession g_i2pSession;

std::string CI2PSession::FormatHelloMessage()
{
    std::ostringstream ss;
    ss << "HELLO VERSION"
       << " MIN=" << I2P_SAM_PROTOCOL_MAJOR << "." << I2P_SAM_PROTOCOL_MINOR
       << " MAX=" << I2P_SAM_PROTOCOL_MAJOR << "." << I2P_SAM_PROTOCOL_MINOR
       << "\n";
    return ss.str();
}

std::string CI2PSession::FormatSessionCreateMessage(const std::string& strSessionID,
                                                     const std::string& strStyle)
{
    return "SESSION CREATE"
           " STYLE=" + std::string(strStyle) +
           " ID=" + strSessionID +
           " DESTINATION=TRANSIENT"
           "\n";
}

std::string CI2PSession::FormatStreamAcceptMessage(const std::string& strSessionID)
{
    return "STREAM ACCEPT ID=" + strSessionID + " SILENT=false\n";
}

std::string CI2PSession::FormatStreamConnectMessage(const std::string& strSessionID,
                                                     const std::string& strDestination)
{
    return "STREAM CONNECT ID=" + strSessionID + " DESTINATION=" + strDestination + " SILENT=false\n";
}

CI2PSession::CI2PSession()
    : nState(I2P_DISCONNECTED),
      hSocket(INVALID_SOCKET),
      nSAMPort(I2P_SAM_DEFAULT_PORT),
      fAcceptIncoming(true)
{
}

CI2PSession::~CI2PSession()
{
    Disconnect();
}

std::string CI2PSession::GenerateSessionID()
{

    uint256 hash = GetRandHash();

    return hash.ToString().substr(0, 16);
}

std::string CI2PSession::DestinationToB32(const std::string& strDest)
{

    if (strDest.empty())
        return "";

    unsigned char hash[32];
    SHA256((const unsigned char*)strDest.c_str(), strDest.size(), hash);

    static const char* b32chars = "abcdefghijklmnopqrstuvwxyz234567";
    std::string strB32;
    int nBits = 0;
    int nAccum = 0;

    for (int i = 0; i < 32; ++i) {
        nAccum = (nAccum << 8) | hash[i];
        nBits += 8;
        while (nBits >= 5) {
            nBits -= 5;
            strB32 += b32chars[(nAccum >> nBits) & 0x1F];
        }
    }

    if (nBits > 0) {
        strB32 += b32chars[(nAccum << (5 - nBits)) & 0x1F];
    }

    return strB32 + ".b32.i2p";
}

bool CI2PSession::SendSAMCommand(const std::string& strCommand)
{
    return SendSAMCommand(hSocket, strCommand);
}

bool CI2PSession::SendSAMCommand(SOCKET hSock, const std::string& strCommand)
{
    if (hSock == INVALID_SOCKET)
        return false;

    int nSent = send(hSock, strCommand.c_str(), strCommand.size(), 0);
    if (nSent != (int)strCommand.size()) {
        printf("I2P: send failed (%d bytes of %d)\n", nSent, (int)strCommand.size());
        return false;
    }

    if (fDebug)
        printf("I2P: >> %s", strCommand.c_str());

    return true;
}

bool CI2PSession::ReadSAMReply(std::string& strReply)
{
    return ReadSAMReply(hSocket, strReply);
}

bool CI2PSession::ReadSAMReply(SOCKET hSock, std::string& strReply)
{
    if (hSock == INVALID_SOCKET)
        return false;

    strReply.clear();
    char ch;

    while (true) {
        int nRecv = recv(hSock, &ch, 1, 0);
        if (nRecv <= 0) {
            printf("I2P: recv failed during SAM reply\n");
            return false;
        }
        if (ch == '\n') break;
        strReply += ch;
    }

    if (fDebug)
        printf("I2P: << %s\n", strReply.c_str());

    return true;
}

bool CI2PSession::ParseSAMReply(const std::string& strReply,
                                 std::string& strCmd,
                                 std::string& strResult,
                                 std::map<std::string, std::string>& mapParams)
{
    strCmd.clear();
    strResult.clear();
    mapParams.clear();

    std::istringstream iss(strReply);
    std::string strToken;

    if (!(iss >> strToken)) return false;
    strCmd = strToken;

    if (!(iss >> strToken)) return false;
    strCmd += " " + strToken;

    while (iss >> strToken) {
        size_t eq = strToken.find('=');
        if (eq != std::string::npos) {
            std::string key = strToken.substr(0, eq);
            std::string val = strToken.substr(eq + 1);
            mapParams[key] = val;

            if (key == "RESULT")
                strResult = val;
        }
    }

    return true;
}

bool CI2PSession::Connect(const std::string& strHost, int nPort)
{
    LOCK(cs_i2p);

    if (hSocket != INVALID_SOCKET)
        Disconnect();

    strSAMHost = strHost;
    nSAMPort = nPort;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nPort);

    struct hostent* he = gethostbyname(strHost.c_str());
    if (!he) {
        printf("I2P: cannot resolve SAM host '%s'\n", strHost.c_str());
        nState = I2P_ERROR;
        return false;
    }
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSocket == INVALID_SOCKET) {
        printf("I2P: socket() failed\n");
        nState = I2P_ERROR;
        return false;
    }

    if (connect(hSocket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("I2P: connect to SAM bridge %s:%d failed\n", strHost.c_str(), nPort);
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
        nState = I2P_ERROR;
        return false;
    }

    printf("I2P: connected to SAM bridge %s:%d\n", strHost.c_str(), nPort);

    nState = I2P_HELLO_SENT;
    if (!SendSAMCommand(FormatHelloMessage())) {
        nState = I2P_ERROR;
        return false;
    }

    std::string strReply;
    if (!ReadSAMReply(strReply)) {
        nState = I2P_ERROR;
        return false;
    }

    std::string strCmd, strResult;
    std::map<std::string, std::string> mapParams;
    if (!ParseSAMReply(strReply, strCmd, strResult, mapParams)) {
        printf("I2P: failed to parse HELLO reply\n");
        nState = I2P_ERROR;
        return false;
    }

    if (strResult != "OK") {
        printf("I2P: HELLO failed: RESULT=%s\n", strResult.c_str());
        nState = I2P_ERROR;
        return false;
    }

    printf("I2P: HELLO succeeded, SAM version %s\n",
           mapParams.count("VERSION") ? mapParams["VERSION"].c_str() : "?");

    nState = I2P_HELLO_OK;
    return true;
}

bool CI2PSession::CreateSession(const std::string& strStyle)
{
    LOCK(cs_i2p);

    if (nState != I2P_HELLO_OK)
        return false;

    strSessionID = GenerateSessionID();

    std::string strCmd = FormatSessionCreateMessage(strSessionID, strStyle);
    if (!SendSAMCommand(strCmd)) {
        nState = I2P_ERROR;
        return false;
    }

    std::string strReply;
    if (!ReadSAMReply(strReply)) {
        nState = I2P_ERROR;
        return false;
    }

    std::string strParsedCmd, strResult;
    std::map<std::string, std::string> mapParams;
    if (!ParseSAMReply(strReply, strParsedCmd, strResult, mapParams)) {
        printf("I2P: failed to parse SESSION STATUS reply\n");
        nState = I2P_ERROR;
        return false;
    }

    if (strResult != "OK") {
        printf("I2P: SESSION CREATE failed: RESULT=%s\n", strResult.c_str());
        if (mapParams.count("MESSAGE"))
            printf("I2P: MESSAGE=%s\n", mapParams["MESSAGE"].c_str());
        nState = I2P_ERROR;
        return false;
    }

    if (mapParams.count("DESTINATION")) {
        strMyDestination = mapParams["DESTINATION"];
        strMyDestinationB32 = DestinationToB32(strMyDestination);
    }

    printf("I2P: session created, ID=%s\n", strSessionID.c_str());
    if (!strMyDestinationB32.empty())
        printf("I2P: our address: %s\n", strMyDestinationB32.c_str());

    nState = I2P_SESSION_CREATED;
    return true;
}

bool CI2PSession::Accept(SOCKET& hPeerSocket, std::string& strPeerDest)
{
    LOCK(cs_i2p);

    if (nState != I2P_SESSION_CREATED && nState != I2P_ACCEPTING)
        return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nSAMPort);

    struct hostent* he = gethostbyname(strSAMHost.c_str());
    if (!he) return false;
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    SOCKET hAcceptSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hAcceptSock == INVALID_SOCKET)
        return false;

    if (connect(hAcceptSock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(hAcceptSock);
        return false;
    }

    if (!SendSAMCommand(hAcceptSock, FormatHelloMessage())) {
        closesocket(hAcceptSock);
        return false;
    }

    std::string strHelloReply;
    if (!ReadSAMReply(hAcceptSock, strHelloReply)) {
        closesocket(hAcceptSock);
        return false;
    }

    nState = I2P_ACCEPTING;
    if (!SendSAMCommand(hAcceptSock, FormatStreamAcceptMessage(strSessionID))) {
        closesocket(hAcceptSock);
        return false;
    }

    std::string strReply;
    if (!ReadSAMReply(hAcceptSock, strReply)) {
        closesocket(hAcceptSock);
        return false;
    }

    std::string strCmd, strResult;
    std::map<std::string, std::string> mapParams;
    if (!ParseSAMReply(strReply, strCmd, strResult, mapParams)) {
        closesocket(hAcceptSock);
        return false;
    }

    if (strResult != "OK") {
        printf("I2P: STREAM ACCEPT failed: RESULT=%s\n", strResult.c_str());
        closesocket(hAcceptSock);
        return false;
    }

    std::string strPeerLine;
    if (ReadSAMReply(hAcceptSock, strPeerLine)) {
        strPeerDest = strPeerLine;
    }

    hPeerSocket = hAcceptSock;
    nState = I2P_SESSION_CREATED;
    printf("I2P: accepted inbound connection from %s\n",
           strPeerDest.substr(0, 16).c_str());
    return true;
}

bool CI2PSession::ConnectTo(const std::string& strDestination, SOCKET& hConnSocket)
{
    LOCK(cs_i2p);

    if (nState != I2P_SESSION_CREATED)
        return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nSAMPort);

    struct hostent* he = gethostbyname(strSAMHost.c_str());
    if (!he) return false;
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    SOCKET hSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSock == INVALID_SOCKET)
        return false;

    if (connect(hSock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(hSock);
        return false;
    }

    if (!SendSAMCommand(hSock, FormatHelloMessage())) {
        closesocket(hSock);
        return false;
    }

    std::string strHelloReply;
    if (!ReadSAMReply(hSock, strHelloReply)) {
        closesocket(hSock);
        return false;
    }

    if (!SendSAMCommand(hSock, FormatStreamConnectMessage(strSessionID, strDestination))) {
        closesocket(hSock);
        return false;
    }

    std::string strReply;
    if (!ReadSAMReply(hSock, strReply)) {
        closesocket(hSock);
        return false;
    }

    std::string strCmd, strResult;
    std::map<std::string, std::string> mapParams;
    if (!ParseSAMReply(strReply, strCmd, strResult, mapParams)) {
        closesocket(hSock);
        return false;
    }

    if (strResult != "OK") {
        printf("I2P: STREAM CONNECT to %s failed: RESULT=%s\n",
               strDestination.substr(0, 16).c_str(), strResult.c_str());
        if (mapParams.count("MESSAGE"))
            printf("I2P: MESSAGE=%s\n", mapParams["MESSAGE"].c_str());
        closesocket(hSock);
        return false;
    }

    hConnSocket = hSock;
    printf("I2P: connected to %s\n", strDestination.substr(0, 16).c_str());
    return true;
}

void CI2PSession::Disconnect()
{
    LOCK(cs_i2p);

    if (hSocket != INVALID_SOCKET) {
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
    }

    nState = I2P_DISCONNECTED;
    strSessionID.clear();
    strMyDestination.clear();
    strMyDestinationB32.clear();
}

std::string CI2PSession::GetMyDestination() const
{
    LOCK(cs_i2p);
    return strMyDestination;
}

std::string CI2PSession::GetMyDestinationB32() const
{
    LOCK(cs_i2p);
    return strMyDestinationB32;
}

I2PSessionState CI2PSession::GetState() const
{
    LOCK(cs_i2p);
    return nState;
}

bool CI2PSession::IsReady() const
{
    LOCK(cs_i2p);
    return nState == I2P_SESSION_CREATED;
}

bool CI2PSession::IsI2PRunning(const std::string& strHost, int nPort)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nPort);

    struct hostent* he = gethostbyname(strHost.c_str());
    if (!he) return false;
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    SOCKET hSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSock == INVALID_SOCKET)
        return false;

#ifdef WIN32
    int nTimeout = 1000;
    setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(nTimeout));
    setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&nTimeout, sizeof(nTimeout));
#else
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(hSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    bool fRunning = (connect(hSock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    closesocket(hSock);
    return fRunning;
}

void InitI2P()
{
    std::string strSAMAddr = GetArg("-i2psam", "");
    if (strSAMAddr.empty())
        return;

    bool fAcceptIncoming = GetBoolArg("-i2pacceptincoming", true);

    std::string strHost = I2P_SAM_DEFAULT_HOST;
    int nPort = I2P_SAM_DEFAULT_PORT;

    int nParsedPort = 0;
    std::string strParsedHost;
    SplitHostPort(strSAMAddr, nParsedPort, strParsedHost);
    if (!strParsedHost.empty())
        strHost = strParsedHost;
    if (nParsedPort > 0)
        nPort = nParsedPort;

    printf("I2P: connecting to SAM bridge at %s:%d\n", strHost.c_str(), nPort);

    if (!g_i2pSession.Connect(strHost, nPort)) {
        printf("I2P: failed to connect to SAM bridge\n");
        return;
    }

    if (!g_i2pSession.CreateSession()) {
        printf("I2P: failed to create SAM session\n");
        return;
    }

    std::string strB32 = g_i2pSession.GetMyDestinationB32();
    if (!strB32.empty()) {
        printf("I2P: our I2P address: %s\n", strB32.c_str());

        SetReachable(NET_I2P);

        CNetAddr i2pAddr;
        if (i2pAddr.SetSpecial(strB32)) {
            AddLocal(i2pAddr, LOCAL_MANUAL);
        }
    }

    if (fAcceptIncoming) {
        printf("I2P: accepting incoming connections\n");

    }
}

void ShutdownI2P()
{
    g_i2pSession.Disconnect();
}
