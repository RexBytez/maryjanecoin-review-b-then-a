#include <support/cleanse.h>

#include <cstring>

#if defined(WIN32)
#include <windows.h>
#endif

void memory_cleanse(void *ptr, size_t len)
{
#if defined(WIN32)

    SecureZeroMemory(ptr, len);
#else
    std::memset(ptr, 0, len);

    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#endif
}
