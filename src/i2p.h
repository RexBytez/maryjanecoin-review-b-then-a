#ifndef MARYJANECOIN_I2P_H
#define MARYJANECOIN_I2P_H

#include <string>
#include <vector>
#include <map>
#include "netbase.h"
#include "sync.h"

static const int    I2P_SAM_DEFAULT_PORT      = 7656;
static const char*  I2P_SAM_DEFAULT_HOST      = "127.0.0.1";
static const int    I2P_SAM_PROTOCOL_MAJOR    = 3;
static const int    I2P_SAM_PROTOCOL_MINOR    = 1;

static const char*  I2P_SESSION_STYLE_STREAM  = "STREAM";

enum I2PSessionState {
    I2P_DISCONNECTED   = 0,
    I2P_HELLO_SENT     = 1,
    I2P_HELLO_OK       = 2,
    I2P_SESSION_CREATED = 3,
    I2P_ACCEPTING      = 4,
    I2P_CONNECTED      = 5,
    I2P_ERROR          = 6,
};

class CI2PSession {
private:
    mutable CCriticalSection cs_i2p;

    I2PSessionState nState;
    SOCKET hSocket;
    std::string strSAMHost;
    int nSAMPort;

    std::string strSessionID;
    std::string strMyDestination;
    std::string strMyDestinationB32;
    bool fAcceptIncoming;

    bool SendSAMCommand(const std::string& strCommand);
    bool SendSAMCommand(SOCKET hSock, const std::string& strCommand);
    bool ReadSAMReply(std::string& strReply);
    bool ReadSAMReply(SOCKET hSock, std::string& strReply);
    bool ParseSAMReply(const std::string& strReply,
                       std::string& strCmd,
                       std::string& strResult,
                       std::map<std::string, std::string>& mapParams);

    std::string GenerateSessionID();

    static std::string DestinationToB32(const std::string& strDest);

public:
    CI2PSession();
    ~CI2PSession();

    bool Connect(const std::string& strHost = I2P_SAM_DEFAULT_HOST,
                 int nPort = I2P_SAM_DEFAULT_PORT);

    bool CreateSession(const std::string& strStyle = I2P_SESSION_STYLE_STREAM);

    bool Accept(SOCKET& hPeerSocket, std::string& strPeerDest);

    bool ConnectTo(const std::string& strDestination, SOCKET& hConnSocket);

    void Disconnect();

    std::string GetMyDestination() const;

    std::string GetMyDestinationB32() const;

    I2PSessionState GetState() const;

    bool IsReady() const;

    static bool IsI2PRunning(const std::string& strHost = I2P_SAM_DEFAULT_HOST,
                             int nPort = I2P_SAM_DEFAULT_PORT);

    static std::string FormatHelloMessage();

    static std::string FormatSessionCreateMessage(const std::string& strSessionID,
                                                  const std::string& strStyle = I2P_SESSION_STYLE_STREAM);

    static std::string FormatStreamAcceptMessage(const std::string& strSessionID);

    static std::string FormatStreamConnectMessage(const std::string& strSessionID,
                                                  const std::string& strDestination);
};

extern CI2PSession g_i2pSession;

void InitI2P();

void ShutdownI2P();

#endif
