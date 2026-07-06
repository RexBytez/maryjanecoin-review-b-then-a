#include <support/lockedpool.h>
#include <support/cleanse.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/resource.h>
#include <limits.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#ifdef ARENA_DEBUG
#include <iomanip>
#include <iostream>
#endif

LockedPoolManager* LockedPoolManager::_instance = nullptr;

static inline size_t align_up(size_t x, size_t align)
{
    return (x + align - 1) & ~(align - 1);
}

Arena::Arena(void *base_in, size_t size_in, size_t alignment_in):
    base(base_in), end(static_cast<char*>(base_in) + size_in), alignment(alignment_in)
{

    auto it = size_to_free_chunk.emplace(size_in, base);
    chunks_free.emplace(base, it);
    chunks_free_end.emplace(static_cast<char*>(base) + size_in, it);
}

Arena::~Arena() = default;

void* Arena::alloc(size_t size)
{

    size = align_up(size, alignment);

    if (size == 0)
        return nullptr;

    auto size_ptr_it = size_to_free_chunk.lower_bound(size);
    if (size_ptr_it == size_to_free_chunk.end())
        return nullptr;

    const size_t size_remaining = size_ptr_it->first - size;
    char* const free_chunk = static_cast<char*>(size_ptr_it->second);
    auto allocated = chunks_used.emplace(free_chunk + size_remaining, size).first;
    chunks_free_end.erase(free_chunk + size_ptr_it->first);
    if (size_ptr_it->first == size) {

        chunks_free.erase(size_ptr_it->second);
    } else {

        auto it_remaining = size_to_free_chunk.emplace(size_remaining, size_ptr_it->second);
        chunks_free[size_ptr_it->second] = it_remaining;
        chunks_free_end.emplace(free_chunk + size_remaining, it_remaining);
    }
    size_to_free_chunk.erase(size_ptr_it);

    return allocated->first;
}

void Arena::free(void *ptr)
{

    if (ptr == nullptr) {
        return;
    }

    auto i = chunks_used.find(ptr);
    if (i == chunks_used.end()) {
        throw std::runtime_error("Arena: invalid or double free");
    }
    auto freed = std::make_pair(static_cast<char*>(i->first), i->second);
    chunks_used.erase(i);

    auto prev = chunks_free_end.find(freed.first);
    if (prev != chunks_free_end.end()) {
        freed.first -= prev->second->first;
        freed.second += prev->second->first;
        size_to_free_chunk.erase(prev->second);
        chunks_free_end.erase(prev);
    }

    auto next = chunks_free.find(freed.first + freed.second);
    if (next != chunks_free.end()) {
        freed.second += next->second->first;
        size_to_free_chunk.erase(next->second);
        chunks_free.erase(next);
    }

    auto it = size_to_free_chunk.emplace(freed.second, freed.first);
    chunks_free[freed.first] = it;
    chunks_free_end[freed.first + freed.second] = it;
}

Arena::Stats Arena::stats() const
{
    Arena::Stats r{ 0, 0, 0, chunks_used.size(), chunks_free.size() };
    for (const auto& chunk: chunks_used)
        r.used += chunk.second;
    for (const auto& chunk: chunks_free)
        r.free += chunk.second->first;
    r.total = r.used + r.free;
    return r;
}

#ifdef ARENA_DEBUG
static void printchunk(void* base, size_t sz, bool used) {
    std::cout <<
        "0x" << std::hex << std::setw(16) << std::setfill('0') << base <<
        " 0x" << std::hex << std::setw(16) << std::setfill('0') << sz <<
        " 0x" << used << std::endl;
}
void Arena::walk() const
{
    for (const auto& chunk: chunks_used)
        printchunk(chunk.first, chunk.second, true);
    std::cout << std::endl;
    for (const auto& chunk: chunks_free)
        printchunk(chunk.first, chunk.second->first, false);
    std::cout << std::endl;
}
#endif

#ifdef WIN32

class Win32LockedPageAllocator: public LockedPageAllocator
{
public:
    Win32LockedPageAllocator();
    void* AllocateLocked(size_t len, bool *lockingSuccess) override;
    void FreeLocked(void* addr, size_t len) override;
    size_t GetLimit() override;
private:
    size_t page_size;
};

Win32LockedPageAllocator::Win32LockedPageAllocator()
{

    SYSTEM_INFO sSysInfo;
    GetSystemInfo(&sSysInfo);
    page_size = sSysInfo.dwPageSize;
}
void *Win32LockedPageAllocator::AllocateLocked(size_t len, bool *lockingSuccess)
{
    len = align_up(len, page_size);
    void *addr = VirtualAlloc(nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (addr) {

        *lockingSuccess = VirtualLock(const_cast<void*>(addr), len) != 0;
    }
    return addr;
}
void Win32LockedPageAllocator::FreeLocked(void* addr, size_t len)
{
    len = align_up(len, page_size);
    memory_cleanse(addr, len);
    VirtualUnlock(const_cast<void*>(addr), len);
}

size_t Win32LockedPageAllocator::GetLimit()
{
    size_t min, max;
    if(GetProcessWorkingSetSize(GetCurrentProcess(), &min, &max) != 0) {
        return min;
    }
    return std::numeric_limits<size_t>::max();
}
#endif

#ifndef WIN32

class PosixLockedPageAllocator: public LockedPageAllocator
{
public:
    PosixLockedPageAllocator();
    void* AllocateLocked(size_t len, bool *lockingSuccess) override;
    void FreeLocked(void* addr, size_t len) override;
    size_t GetLimit() override;
private:
    size_t page_size;
};

PosixLockedPageAllocator::PosixLockedPageAllocator()
{

#if defined(PAGESIZE)
    page_size = PAGESIZE;
#else
    page_size = sysconf(_SC_PAGESIZE);
#endif
}

void *PosixLockedPageAllocator::AllocateLocked(size_t len, bool *lockingSuccess)
{
    void *addr;
    len = align_up(len, page_size);
    addr = mmap(nullptr, len, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
        return nullptr;
    }
    if (addr) {
        *lockingSuccess = mlock(addr, len) == 0;
#if defined(MADV_DONTDUMP)
        madvise(addr, len, MADV_DONTDUMP);
#elif defined(MADV_NOCORE)
        madvise(addr, len, MADV_NOCORE);
#endif
    }
    return addr;
}
void PosixLockedPageAllocator::FreeLocked(void* addr, size_t len)
{
    len = align_up(len, page_size);
    memory_cleanse(addr, len);
    munlock(addr, len);
    munmap(addr, len);
}
size_t PosixLockedPageAllocator::GetLimit()
{
#ifdef RLIMIT_MEMLOCK
    struct rlimit rlim;
    if (getrlimit(RLIMIT_MEMLOCK, &rlim) == 0) {
        if (rlim.rlim_cur != RLIM_INFINITY) {
            return rlim.rlim_cur;
        }
    }
#endif
    return std::numeric_limits<size_t>::max();
}
#endif

LockedPool::LockedPool(std::unique_ptr<LockedPageAllocator> allocator_in, LockingFailed_Callback lf_cb_in)
    : allocator(std::move(allocator_in)), lf_cb(lf_cb_in)
{
}

LockedPool::~LockedPool() = default;

void* LockedPool::alloc(size_t size)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (size == 0 || size > ARENA_SIZE)
        return nullptr;

    for (auto &arena: arenas) {
        void *addr = arena.alloc(size);
        if (addr) {
            return addr;
        }
    }

    if (new_arena(ARENA_SIZE, ARENA_ALIGN)) {
        return arenas.back().alloc(size);
    }
    return nullptr;
}

void LockedPool::free(void *ptr)
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto &arena: arenas) {
        if (arena.addressInArena(ptr)) {
            arena.free(ptr);
            return;
        }
    }
    throw std::runtime_error("LockedPool: invalid address not pointing to any arena");
}

LockedPool::Stats LockedPool::stats() const
{
    std::lock_guard<std::mutex> lock(mutex);
    LockedPool::Stats r{0, 0, 0, cumulative_bytes_locked, 0, 0};
    for (const auto &arena: arenas) {
        Arena::Stats i = arena.stats();
        r.used += i.used;
        r.free += i.free;
        r.total += i.total;
        r.chunks_used += i.chunks_used;
        r.chunks_free += i.chunks_free;
    }
    return r;
}

bool LockedPool::new_arena(size_t size, size_t align)
{
    bool locked;

    if (arenas.empty()) {
        size_t limit = allocator->GetLimit();
        if (limit > 0) {
            size = std::min(size, limit);
        }
    }
    void *addr = allocator->AllocateLocked(size, &locked);
    if (!addr) {
        return false;
    }
    if (locked) {
        cumulative_bytes_locked += size;
    } else if (lf_cb) {
        if (!lf_cb()) {
            allocator->FreeLocked(addr, size);
            return false;
        }
    }
    arenas.emplace_back(allocator.get(), addr, size, align);
    return true;
}

LockedPool::LockedPageArena::LockedPageArena(LockedPageAllocator *allocator_in, void *base_in, size_t size_in, size_t align_in):
    Arena(base_in, size_in, align_in), base(base_in), size(size_in), allocator(allocator_in)
{
}
LockedPool::LockedPageArena::~LockedPageArena()
{
    allocator->FreeLocked(base, size);
}

LockedPoolManager::LockedPoolManager(std::unique_ptr<LockedPageAllocator> allocator_in):
    LockedPool(std::move(allocator_in), &LockedPoolManager::LockingFailed)
{
}

bool LockedPoolManager::LockingFailed()
{

    return true;
}

void LockedPoolManager::CreateInstance()
{

#ifdef WIN32
    std::unique_ptr<LockedPageAllocator> allocator(new Win32LockedPageAllocator());
#else
    std::unique_ptr<LockedPageAllocator> allocator(new PosixLockedPageAllocator());
#endif
    static LockedPoolManager instance(std::move(allocator));
    LockedPoolManager::_instance = &instance;
}
