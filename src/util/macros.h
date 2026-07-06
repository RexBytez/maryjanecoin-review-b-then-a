#ifndef BITCOIN_UTIL_MACROS_H
#define BITCOIN_UTIL_MACROS_H

#define PASTE(x, y) x ## y
#define PASTE2(x, y) PASTE(x, y)

#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

#endif
