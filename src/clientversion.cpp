#include "clientversion.h"

#include "tinyformat.h"

#include <sstream>
#include <string>
#include <vector>

const std::string CLIENT_NAME("MaryJaneCoin");

#ifdef HAVE_BUILD_INFO
#include "obj/build.h"

#endif

#define GIT_COMMIT_ID "ea9a5c9db5817e7ba8a36af75b16b7f1dc9f8c82"

#ifndef PACKAGE_VERSION
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define PACKAGE_VERSION STRINGIFY(CLIENT_VERSION_MAJOR) "." STRINGIFY(CLIENT_VERSION_MINOR) "." STRINGIFY(CLIENT_VERSION_BUILD)
#endif

#ifdef BUILD_GIT_TAG
    #define BUILD_DESC BUILD_GIT_TAG
    #define BUILD_SUFFIX ""
#else
    #define BUILD_DESC "v" PACKAGE_VERSION
    #if CLIENT_VERSION_IS_RELEASE
        #define BUILD_SUFFIX ""
    #elif defined(BUILD_GIT_COMMIT)
        #define BUILD_SUFFIX "-" BUILD_GIT_COMMIT
    #elif defined(GIT_COMMIT_ID)
        #define BUILD_SUFFIX "-g" GIT_COMMIT_ID
    #else
        #define BUILD_SUFFIX "-unk"
    #endif
#endif

#ifndef BUILD_DATE
#    ifdef GIT_COMMIT_DATE
#        define BUILD_DATE GIT_COMMIT_DATE
#    else
#        define BUILD_DATE __DATE__ ", " __TIME__
#    endif
#endif

const std::string CLIENT_BUILD(BUILD_DESC BUILD_SUFFIX);
const std::string CLIENT_DATE(BUILD_DATE);

static std::string FormatVersion(int nVersion)
{
    return strprintf("%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100);
}

std::string FormatFullVersion()
{
    static const std::string CLIENT_BUILD(BUILD_DESC BUILD_SUFFIX);
    return CLIENT_BUILD;
}

std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments)
{
    std::ostringstream ss;
    ss << "/";
    ss << name << ":" << FormatVersion(nClientVersion);
    if (!comments.empty())
    {
        std::vector<std::string>::const_iterator it(comments.begin());
        ss << "(" << *it;
        for(++it; it != comments.end(); ++it)
            ss << "; " << *it;
        ss << ")";
    }
    ss << "/";
    return ss.str();
}
