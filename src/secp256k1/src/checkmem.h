#ifndef SECP256K1_CHECKMEM_H
#define SECP256K1_CHECKMEM_H

#define SECP256K1_CHECKMEM_NOOP(p, len) do { (void)(p); (void)(len); } while(0)

#if defined(__has_feature)
#  if __has_feature(memory_sanitizer)
#    include <sanitizer/msan_interface.h>
#    define SECP256K1_CHECKMEM_ENABLED 1
#    define SECP256K1_CHECKMEM_UNDEFINE(p, len) __msan_allocated_memory((p), (len))
#    define SECP256K1_CHECKMEM_DEFINE(p, len) __msan_unpoison((p), (len))
#    define SECP256K1_CHECKMEM_MSAN_DEFINE(p, len) __msan_unpoison((p), (len))
#    define SECP256K1_CHECKMEM_CHECK(p, len) __msan_check_mem_is_initialized((p), (len))
#    define SECP256K1_CHECKMEM_RUNNING() (1)
#  endif
#endif

#if !defined SECP256K1_CHECKMEM_MSAN_DEFINE
#  define SECP256K1_CHECKMEM_MSAN_DEFINE(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#endif

#if !defined SECP256K1_CHECKMEM_ENABLED
#  if defined VALGRIND
#    include <stddef.h>
#  if defined(__clang__) && defined(__APPLE__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wreserved-identifier"
#  endif
#    include <valgrind/memcheck.h>
#  if defined(__clang__) && defined(__APPLE__)
#    pragma clang diagnostic pop
#  endif
#    define SECP256K1_CHECKMEM_ENABLED 1
#    define SECP256K1_CHECKMEM_UNDEFINE(p, len) VALGRIND_MAKE_MEM_UNDEFINED((p), (len))
#    define SECP256K1_CHECKMEM_DEFINE(p, len) VALGRIND_MAKE_MEM_DEFINED((p), (len))
#    define SECP256K1_CHECKMEM_CHECK(p, len) VALGRIND_CHECK_MEM_IS_DEFINED((p), (len))

#    define SECP256K1_CHECKMEM_RUNNING() (VALGRIND_MAKE_MEM_DEFINED(NULL, 0) != 0)
#  endif
#endif

#if !defined SECP256K1_CHECKMEM_ENABLED
#  define SECP256K1_CHECKMEM_ENABLED 0
#  define SECP256K1_CHECKMEM_UNDEFINE(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#  define SECP256K1_CHECKMEM_DEFINE(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#  define SECP256K1_CHECKMEM_CHECK(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#  define SECP256K1_CHECKMEM_RUNNING() (0)
#endif

#if defined VERIFY
#define SECP256K1_CHECKMEM_CHECK_VERIFY(p, len) SECP256K1_CHECKMEM_CHECK((p), (len))
#else
#define SECP256K1_CHECKMEM_CHECK_VERIFY(p, len) SECP256K1_CHECKMEM_NOOP((p), (len))
#endif

#endif
