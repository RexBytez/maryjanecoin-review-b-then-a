#ifndef _MINISKETCH_UTIL_H_
#define _MINISKETCH_UTIL_H_

#ifdef MINISKETCH_VERIFY
#include <stdio.h>
#endif

#if !defined(__GNUC_PREREQ)
# if defined(__GNUC__)&&defined(__GNUC_MINOR__)
#  define __GNUC_PREREQ(_maj,_min) \
 ((__GNUC__<<16)+__GNUC_MINOR__>=((_maj)<<16)+(_min))
# else
#  define __GNUC_PREREQ(_maj,_min) 0
# endif
#endif

#if __GNUC_PREREQ(3, 0)
#define EXPECT(x,c) __builtin_expect((x),(c))
#else
#define EXPECT(x,c) (x)
#endif

#define CHECK(cond) do { \
    if (EXPECT(!(cond), 0)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, "Check condition failed: " #cond); \
        abort(); \
    } \
} while(0)

#ifdef MINISKETCH_VERIFY
#define CHECK_SAFE(cond) CHECK(cond)
#else
#define CHECK_SAFE(cond)
#endif

#ifdef MINISKETCH_VERIFY
#define CHECK_RETURN(cond, rvar) do { \
    if (EXPECT(!(cond), 0)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, "Check condition failed: " #cond); \
        abort(); \
        return rvar;  \
    } \
} while(0)
#else
#define CHECK_RETURN(cond, rvar) do { \
    if (EXPECT(!(cond), 0)) { \
        return rvar; \
    } \
} while(0)
#endif

#endif
