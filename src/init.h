#ifndef BITCOIN_INIT_H
#define BITCOIN_INIT_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include "wallet.h"

extern CWallet* pwalletMain;
extern std::string strWalletFileName;
void StartShutdown();
void Shutdown(void* parg);
bool AppInit2();
std::string HelpMessage();

#endif
