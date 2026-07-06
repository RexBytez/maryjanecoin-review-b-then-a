#ifndef MARYJANECOIN_TORCONTROL_H
#define MARYJANECOIN_TORCONTROL_H

#include <string>
#include <vector>
#include "netbase.h"
#include "sync.h"

static const int TOR_DEFAULT_SOCKS_PORT   = 9050;
static const int TOR_DEFAULT_CONTROL_PORT = 9051;

enum TorControlState {
    TOR_DISCONNECTED = 0,
    TOR_CONNECTING   = 1,
    TOR_AUTHENTICATING = 2,
    TOR_AUTHENTICATED  = 3,
    TOR_SERVICE_CREATED = 4,
    TOR_ERROR        = 5,
};

struct CTorControlReply {
    int nCode;
    std::string strStatus;
    std::vector<std::string> vLines;

    CTorControlReply() : nCode(0) {}
};

class CTorControl {
private:
    mutable CCriticalSection cs_tor;

    TorControlState nState;
    SOCKET hSocket;
    std::string strControlHost;
    int nControlPort;

    std::string strAuthCookie;
    std::string strPassword;

    std::string strOnionAddress;
    std::string strServiceID;
    int nOnionPort;

    bool SendCommand(const std::string& strCommand);
    bool ReadReply(CTorControlReply& reply);
    bool ParseReplyLine(const std::string& strLine, CTorControlReply& reply);

    bool AuthenticateCookie();
    bool AuthenticatePassword();
    bool DetectAuthMethod(std::string& strMethod, std::string& strCookieFile);

    bool ReadCookieFile(const std::string& strPath, std::vector<unsigned char>& vchCookie);

public:
    CTorControl();
    ~CTorControl();

    bool Connect(const std::string& strHost = "127.0.0.1", int nPort = TOR_DEFAULT_CONTROL_PORT);

    bool Authenticate(const std::string& strPassword = "");

    bool CreateHiddenService(int nExternalPort, int nInternalPort);

    void Disconnect();

    std::string GetOnionAddress() const;

    TorControlState GetState() const;

    bool IsReady() const;

    static bool IsTorRunning(const std::string& strHost = "127.0.0.1", int nPort = TOR_DEFAULT_SOCKS_PORT);

    static std::string FormatAuthenticateCommand(const std::vector<unsigned char>& vchCookie);

    static std::string FormatAuthenticatePasswordCommand(const std::string& strPassword);

    static std::string FormatProtocolInfoCommand();

    static std::string FormatAddOnionCommand(int nExternalPort, int nInternalPort);

    static std::string FormatDelOnionCommand(const std::string& strServiceID);
};

extern CTorControl g_torControl;

void InitTorControl();

void ShutdownTorControl();

#endif
