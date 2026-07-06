#ifndef BITCOIN_CLIENTVERSION_H
#define BITCOIN_CLIENTVERSION_H

#include <util/macros.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#if !defined(CLIENT_VERSION_MAJOR)
#define CLIENT_VERSION_MAJOR       4
#define CLIENT_VERSION_MINOR       2
#define CLIENT_VERSION_BUILD       0
#define CLIENT_VERSION_IS_RELEASE  true
#define COPYRIGHT_YEAR             2026
#endif

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

#if !defined(WINDRES_PREPROC)

#include <string>
#include <vector>

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;

std::string FormatFullVersion();
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments);

#endif

#endif
