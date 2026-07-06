#ifndef BITCOIN_SUPPORT_ALLOCATORS_POOL_H
#define BITCOIN_SUPPORT_ALLOCATORS_POOL_H

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <list>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

template <std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
class PoolResource final
{
    static_assert(ALIGN_BYTES > 0, "ALIGN_BYTES must be nonzero");
    static_assert((ALIGN_BYTES & (ALIGN_BYTES - 1)) == 0, "ALIGN_BYTES must be a power of two");

    struct ListNode {
        ListNode* m_next;

        explicit ListNode(ListNode* next) : m_next(next) {}
    };
    static_assert(std::is_trivially_destructible<ListNode>::value, "Make sure we don't need to manually call a destructor");

    static constexpr std::size_t ELEM_ALIGN_BYTES = (alignof(ListNode) > ALIGN_BYTES) ? alignof(ListNode) : ALIGN_BYTES;
    static_assert((ELEM_ALIGN_BYTES & (ELEM_ALIGN_BYTES - 1)) == 0, "ELEM_ALIGN_BYTES must be a power of two");
    static_assert(sizeof(ListNode) <= ELEM_ALIGN_BYTES, "Units of size ELEM_SIZE_ALIGN need to be able to store a ListNode");
    static_assert((MAX_BLOCK_SIZE_BYTES & (ELEM_ALIGN_BYTES - 1)) == 0, "MAX_BLOCK_SIZE_BYTES needs to be a multiple of the alignment.");

    const size_t m_chunk_size_bytes;

    std::list<char*> m_allocated_chunks;

    std::array<ListNode*, MAX_BLOCK_SIZE_BYTES / ELEM_ALIGN_BYTES + 1> m_free_lists;

    char* m_available_memory_it;

    char* m_available_memory_end;

    static constexpr std::size_t NumElemAlignBytes(std::size_t bytes)
    {
        return (bytes + ELEM_ALIGN_BYTES - 1) / ELEM_ALIGN_BYTES + (bytes == 0);
    }

    static constexpr bool IsFreeListUsable(std::size_t bytes, std::size_t alignment)
    {
        return alignment <= ELEM_ALIGN_BYTES && bytes <= MAX_BLOCK_SIZE_BYTES;
    }

    void PlacementAddToList(void* p, ListNode*& node)
    {
        node = new (p) ListNode{node};
    }

    static void* AlignedAlloc(std::size_t size, std::size_t alignment)
    {
#ifdef _WIN32
        return _aligned_malloc(size, alignment);
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return nullptr;
        }
        return ptr;
#endif
    }

    static void AlignedFree(void* ptr)
    {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    void AllocateChunk()
    {

        size_t remaining_available_bytes = static_cast<size_t>(m_available_memory_end - m_available_memory_it);
        if (0 != remaining_available_bytes) {
            PlacementAddToList(m_available_memory_it, m_free_lists[remaining_available_bytes / ELEM_ALIGN_BYTES]);
        }

        void* storage = AlignedAlloc(m_chunk_size_bytes, ELEM_ALIGN_BYTES);
        if (!storage) throw std::bad_alloc();
        m_available_memory_it = static_cast<char*>(storage);
        m_available_memory_end = m_available_memory_it + m_chunk_size_bytes;
        m_allocated_chunks.emplace_back(m_available_memory_it);
    }

    friend class PoolResourceTester;

public:

    explicit PoolResource(std::size_t chunk_size_bytes)
        : m_chunk_size_bytes(NumElemAlignBytes(chunk_size_bytes) * ELEM_ALIGN_BYTES),
          m_available_memory_it(nullptr),
          m_available_memory_end(nullptr)
    {
        m_free_lists.fill(nullptr);
        assert(m_chunk_size_bytes >= MAX_BLOCK_SIZE_BYTES);
        AllocateChunk();
    }

    PoolResource() : PoolResource(262144) {}

    PoolResource(const PoolResource&) = delete;
    PoolResource& operator=(const PoolResource&) = delete;
    PoolResource(PoolResource&&) = delete;
    PoolResource& operator=(PoolResource&&) = delete;

    ~PoolResource()
    {
        for (char* chunk : m_allocated_chunks) {
            AlignedFree(static_cast<void*>(chunk));
        }
    }

    void* Allocate(std::size_t bytes, std::size_t alignment)
    {
        if (IsFreeListUsable(bytes, alignment)) {
            const std::size_t num_alignments = NumElemAlignBytes(bytes);
            if (nullptr != m_free_lists[num_alignments]) {

                ListNode* result = m_free_lists[num_alignments];
                m_free_lists[num_alignments] = m_free_lists[num_alignments]->m_next;
                return static_cast<void*>(result);
            }

            const std::ptrdiff_t round_bytes = static_cast<std::ptrdiff_t>(num_alignments * ELEM_ALIGN_BYTES);
            if (round_bytes > m_available_memory_end - m_available_memory_it) {

                AllocateChunk();
            }

            char* result = m_available_memory_it;
            m_available_memory_it = m_available_memory_it + round_bytes;
            return static_cast<void*>(result);
        }

        return AlignedAlloc(bytes, alignment);
    }

    void Deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept
    {
        if (IsFreeListUsable(bytes, alignment)) {
            const std::size_t num_alignments = NumElemAlignBytes(bytes);

            PlacementAddToList(p, m_free_lists[num_alignments]);
        } else {

            AlignedFree(p);
        }
    }

    std::size_t NumAllocatedChunks() const
    {
        return m_allocated_chunks.size();
    }

    size_t ChunkSizeBytes() const
    {
        return m_chunk_size_bytes;
    }
};

template <class T, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES = alignof(T)>
class PoolAllocator
{
    PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>* m_resource;

    template <typename U, std::size_t M, std::size_t A>
    friend class PoolAllocator;

public:
    using value_type = T;
    using ResourceType = PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>;

    PoolAllocator(ResourceType* resource) noexcept
        : m_resource(resource)
    {
    }

    PoolAllocator(const PoolAllocator& other) noexcept = default;
    PoolAllocator& operator=(const PoolAllocator& other) noexcept = default;

    template <class U>
    PoolAllocator(const PoolAllocator<U, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& other) noexcept
        : m_resource(other.resource())
    {
    }

    template <typename U>
    struct rebind {
        using other = PoolAllocator<U, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>;
    };

    T* allocate(size_t n)
    {
        return static_cast<T*>(m_resource->Allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, size_t n) noexcept
    {
        m_resource->Deallocate(p, n * sizeof(T), alignof(T));
    }

    ResourceType* resource() const noexcept
    {
        return m_resource;
    }
};

template <class T1, class T2, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
bool operator==(const PoolAllocator<T1, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& a,
                const PoolAllocator<T2, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& b) noexcept
{
    return a.resource() == b.resource();
}

template <class T1, class T2, std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
bool operator!=(const PoolAllocator<T1, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& a,
                const PoolAllocator<T2, MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>& b) noexcept
{
    return !(a == b);
}

#endif
