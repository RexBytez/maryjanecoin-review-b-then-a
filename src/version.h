#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include "clientversion.h"
#include <string>

static const int DATABASE_VERSION = 70508;

static const int PROTOCOL_VERSION = 100001;

static const int PROTOCOL_VERSION_MOBILE = 100001;

static const int MIN_PROTO_VERSION = 100001;

static const int CADDR_TIME_VERSION = 31402;

static const int NOBLKS_VERSION_START = 60002;
static const int NOBLKS_VERSION_END = 60006;

static const int BIP0031_VERSION = 60000;

static const int MEMPOOL_GD_VERSION = 60002;

#endif
