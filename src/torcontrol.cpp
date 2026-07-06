#include "torcontrol.h"
#include "util.h"
#include "net.h"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <fstream>

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

CTorControl g_torControl;

std::string CTorControl::FormatAuthenticateCommand(const std::vector<unsigned char>& vchCookie)
{
    std::ostringstream ss;
    ss << "AUTHENTICATE ";
    for (size_t i = 0; i < vchCookie.size(); ++i) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", vchCookie[i]);
        ss << hex;
    }
    ss << "\r\n";
    return ss.str();
}

std::string CTorControl::FormatAuthenticatePasswordCommand(const std::string& strPassword)
{

    std::string strEscaped;
    for (size_t i = 0; i < strPassword.size(); ++i) {
        if (strPassword[i] == '"' || strPassword[i] == '\\')
            strEscaped += '\\';
        strEscaped += strPassword[i];
    }
    return "AUTHENTICATE \"" + strEscaped + "\"\r\n";
}

std::string CTorControl::FormatProtocolInfoCommand()
{
    return "PROTOCOLINFO 1\r\n";
}

std::string CTorControl::FormatAddOnionCommand(int nExternalPort, int nInternalPort)
{
    std::ostringstream ss;
    ss << "ADD_ONION NEW:BEST Port=" << nExternalPort << ",127.0.0.1:" << nInternalPort << "\r\n";
    return ss.str();
}

std::string CTorControl::FormatDelOnionCommand(const std::string& strServiceID)
{
    return "DEL_ONION " + strServiceID + "\r\n";
}

CTorControl::CTorControl()
    : nState(TOR_DISCONNECTED),
      hSocket(INVALID_SOCKET),
      nControlPort(TOR_DEFAULT_CONTROL_PORT),
      nOnionPort(0)
{
}

CTorControl::~CTorControl()
{
    Disconnect();
}

bool CTorControl::SendCommand(const std::string& strCommand)
{
    if (hSocket == INVALID_SOCKET)
        return false;

    int nSent = send(hSocket, strCommand.c_str(), strCommand.size(), 0);
    if (nSent != (int)strCommand.size()) {
        printf("TorControl: send failed (%d bytes of %d)\n", nSent, (int)strCommand.size());
        return false;
    }

    if (fDebug)
        printf("TorControl: >> %s", strCommand.c_str());

    return true;
}

bool CTorControl::ReadReply(CTorControlReply& reply)
{
    if (hSocket == INVALID_SOCKET)
        return false;

    reply.nCode = 0;
    reply.strStatus.clear();
    reply.vLines.clear();

    std::string strLine;
    bool fDone = false;

    while (!fDone) {
        strLine.clear();
        char ch;
        while (true) {
            int nRecv = recv(hSocket, &ch, 1, 0);
            if (nRecv <= 0) {
                printf("TorControl: recv failed during reply\n");
                return false;
            }
            if (ch == '\n') break;
            if (ch != '\r')
                strLine += ch;
        }

        if (fDebug)
            printf("TorControl: << %s\n", strLine.c_str());

        if (strLine.size() < 4) {
            printf("TorControl: reply line too short: '%s'\n", strLine.c_str());
            return false;
        }

        int nCode = atoi(strLine.substr(0, 3).c_str());
        char cSeparator = strLine[3];

        if (reply.nCode == 0)
            reply.nCode = nCode;

        std::string strContent = strLine.substr(4);

        if (cSeparator == ' ') {

            if (reply.strStatus.empty())
                reply.strStatus = strContent;
            else
                reply.vLines.push_back(strContent);
            fDone = true;
        } else if (cSeparator == '-') {

            reply.vLines.push_back(strContent);
        } else if (cSeparator == '+') {

            reply.vLines.push_back(strContent);

            while (true) {
                std::string strDataLine;
                while (true) {
                    int nRecv = recv(hSocket, &ch, 1, 0);
                    if (nRecv <= 0) return false;
                    if (ch == '\n') break;
                    if (ch != '\r') strDataLine += ch;
                }
                if (strDataLine == ".")
                    break;
                reply.vLines.push_back(strDataLine);
            }
        }
    }

    return true;
}

bool CTorControl::ReadCookieFile(const std::string& strPath, std::vector<unsigned char>& vchCookie)
{
    std::ifstream file(strPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        printf("TorControl: cannot open cookie file '%s'\n", strPath.c_str());
        return false;
    }

    vchCookie.clear();
    char ch;
    while (file.get(ch))
        vchCookie.push_back((unsigned char)ch);

    file.close();

    if (vchCookie.size() != 32) {
        printf("TorControl: cookie file has unexpected size %d (expected 32)\n",
               (int)vchCookie.size());
        return false;
    }

    return true;
}

bool CTorControl::DetectAuthMethod(std::string& strMethod, std::string& strCookieFile)
{

    if (!SendCommand(FormatProtocolInfoCommand()))
        return false;

    CTorControlReply reply;
    if (!ReadReply(reply))
        return false;

    if (reply.nCode != 250) {
        printf("TorControl: PROTOCOLINFO returned code %d\n", reply.nCode);
        return false;
    }

    strMethod = "NULL";
    strCookieFile.clear();

    for (size_t i = 0; i < reply.vLines.size(); ++i) {
        const std::string& line = reply.vLines[i];

        if (line.find("AUTH METHODS=") != std::string::npos) {
            size_t pos = line.find("METHODS=");
            if (pos != std::string::npos) {
                std::string methods = line.substr(pos + 8);

                size_t sp = methods.find(' ');
                if (sp != std::string::npos) {

                    std::string rest = methods.substr(sp);
                    methods = methods.substr(0, sp);

                    size_t cookiePos = rest.find("COOKIEFILE=\"");
                    if (cookiePos != std::string::npos) {
                        size_t start = cookiePos + 12;
                        size_t end = rest.find('"', start);
                        if (end != std::string::npos) {
                            strCookieFile = rest.substr(start, end - start);
                        }
                    }
                }

                if (methods.find("COOKIE") != std::string::npos)
                    strMethod = "COOKIE";
                else if (methods.find("HASHEDPASSWORD") != std::string::npos)
                    strMethod = "HASHEDPASSWORD";
                else
                    strMethod = "NULL";
            }
        }
    }

    return true;
}

bool CTorControl::AuthenticateCookie()
{
    std::string strMethod, strCookieFile;
    if (!DetectAuthMethod(strMethod, strCookieFile))
        return false;

    if (strMethod == "NULL") {

        if (!SendCommand("AUTHENTICATE\r\n"))
            return false;
    } else if (strMethod == "COOKIE" && !strCookieFile.empty()) {
        std::vector<unsigned char> vchCookie;
        if (!ReadCookieFile(strCookieFile, vchCookie))
            return false;
        if (!SendCommand(FormatAuthenticateCommand(vchCookie)))
            return false;
    } else {
        printf("TorControl: cannot authenticate with method '%s'\n", strMethod.c_str());
        return false;
    }

    CTorControlReply reply;
    if (!ReadReply(reply))
        return false;

    if (reply.nCode != 250) {
        printf("TorControl: AUTHENTICATE failed with code %d: %s\n",
               reply.nCode, reply.strStatus.c_str());
        return false;
    }

    return true;
}

bool CTorControl::AuthenticatePassword()
{
    if (!SendCommand(FormatAuthenticatePasswordCommand(strPassword)))
        return false;

    CTorControlReply reply;
    if (!ReadReply(reply))
        return false;

    if (reply.nCode != 250) {
        printf("TorControl: password AUTHENTICATE failed with code %d: %s\n",
               reply.nCode, reply.strStatus.c_str());
        return false;
    }

    return true;
}

bool CTorControl::Connect(const std::string& strHost, int nPort)
{
    LOCK(cs_tor);

    if (hSocket != INVALID_SOCKET)
        Disconnect();

    strControlHost = strHost;
    nControlPort = nPort;
    nState = TOR_CONNECTING;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(nPort);

    struct hostent* he = gethostbyname(strHost.c_str());
    if (!he) {
        printf("TorControl: cannot resolve '%s'\n", strHost.c_str());
        nState = TOR_ERROR;
        return false;
    }
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hSocket == INVALID_SOCKET) {
        printf("TorControl: socket() failed\n");
        nState = TOR_ERROR;
        return false;
    }

    if (connect(hSocket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("TorControl: connect to %s:%d failed\n", strHost.c_str(), nPort);
        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
        nState = TOR_ERROR;
        return false;
    }

    printf("TorControl: connected to %s:%d\n", strHost.c_str(), nPort);
    nState = TOR_AUTHENTICATING;
    return true;
}

bool CTorControl::Authenticate(const std::string& strPass)
{
    LOCK(cs_tor);

    if (hSocket == INVALID_SOCKET || nState < TOR_AUTHENTICATING)
        return false;

    strPassword = strPass;

    if (AuthenticateCookie()) {
        printf("TorControl: authenticated via cookie\n");
        nState = TOR_AUTHENTICATED;
        return true;
    }

    if (!strPassword.empty()) {

        Disconnect();
        if (!Connect(strControlHost, nControlPort))
            return false;

        if (AuthenticatePassword()) {
            printf("TorControl: authenticated via password\n");
            nState = TOR_AUTHENTICATED;
            return true;
        }
    }

    printf("TorControl: all authentication methods failed\n");
    nState = TOR_ERROR;
    return false;
}

bool CTorControl::CreateHiddenService(int nExternalPort, int nInternalPort)
{
    LOCK(cs_tor);

    if (nState != TOR_AUTHENTICATED)
        return false;

    nOnionPort = nInternalPort;

    std::string strCmd = FormatAddOnionCommand(nExternalPort, nInternalPort);
    if (!SendCommand(strCmd))
        return false;

    CTorControlReply reply;
    if (!ReadReply(reply))
        return false;

    if (reply.nCode != 250) {
        printf("TorControl: ADD_ONION failed with code %d: %s\n",
               reply.nCode, reply.strStatus.c_str());
        return false;
    }

    for (size_t i = 0; i < reply.vLines.size(); ++i) {
        const std::string& line = reply.vLines[i];
        if (line.find("ServiceID=") == 0) {
            strServiceID = line.substr(10);
            strOnionAddress = strServiceID + ".onion";
            break;
        }
    }

    if (strServiceID.empty() && reply.strStatus.find("ServiceID=") != std::string::npos) {
        size_t pos = reply.strStatus.find("ServiceID=");
        strServiceID = reply.strStatus.substr(pos + 10);

        size_t sp = strServiceID.find(' ');
        if (sp != std::string::npos)
            strServiceID = strServiceID.substr(0, sp);
        strOnionAddress = strServiceID + ".onion";
    }

    if (strServiceID.empty()) {
        printf("TorControl: ADD_ONION succeeded but ServiceID not found in reply\n");
        return false;
    }

    printf("TorControl: hidden service created at %s (port %d -> 127.0.0.1:%d)\n",
           strOnionAddress.c_str(), nExternalPort, nInternalPort);

    nState = TOR_SERVICE_CREATED;
    return true;
}

void CTorControl::Disconnect()
{
    LOCK(cs_tor);

    if (hSocket != INVALID_SOCKET) {

        if (!strServiceID.empty()) {
            SendCommand(FormatDelOnionCommand(strServiceID));
            CTorControlReply reply;
            ReadReply(reply);
            printf("TorControl: removed hidden service %s\n", strOnionAddress.c_str());
        }

        closesocket(hSocket);
        hSocket = INVALID_SOCKET;
    }

    nState = TOR_DISCONNECTED;
    strServiceID.clear();
    strOnionAddress.clear();
}

std::string CTorControl::GetOnionAddress() const
{
    LOCK(cs_tor);
    return strOnionAddress;
}

TorControlState CTorControl::GetState() const
{
    LOCK(cs_tor);
    return nState;
}

bool CTorControl::IsReady() const
{
    LOCK(cs_tor);
    return nState >= TOR_AUTHENTICATED;
}

bool CTorControl::IsTorRunning(const std::string& strHost, int nPort)
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

void InitTorControl()
{

    bool fTorOnly = GetBoolArg("-toronly", false);
    std::string strControlAddr = GetArg("-torcontrol", "");

    if (strControlAddr.empty() && !fTorOnly)
        return;

    std::string strHost = "127.0.0.1";
    int nPort = TOR_DEFAULT_CONTROL_PORT;

    if (!strControlAddr.empty()) {
        int nParsedPort = 0;
        std::string strParsedHost;
        SplitHostPort(strControlAddr, nParsedPort, strParsedHost);
        if (!strParsedHost.empty())
            strHost = strParsedHost;
        if (nParsedPort > 0)
            nPort = nParsedPort;
    }

    std::string strPassword = GetArg("-torpassword", "");

    if (!g_torControl.Connect(strHost, nPort)) {
        printf("TorControl: failed to connect to Tor control port at %s:%d\n",
               strHost.c_str(), nPort);
        return;
    }

    if (!g_torControl.Authenticate(strPassword)) {
        printf("TorControl: failed to authenticate with Tor\n");
        return;
    }

    unsigned short nListenPort = GetListenPort();
    if (!g_torControl.CreateHiddenService(nListenPort, nListenPort)) {
        printf("TorControl: failed to create hidden service\n");
        return;
    }

    std::string strOnion = g_torControl.GetOnionAddress();
    if (!strOnion.empty()) {
        CService addr(strOnion, nListenPort);
        if (addr.IsValid()) {
            AddLocal(addr, LOCAL_MANUAL);
            printf("TorControl: advertising onion address %s:%d\n",
                   strOnion.c_str(), nListenPort);
        }
    }
}

void ShutdownTorControl()
{
    g_torControl.Disconnect();
}
