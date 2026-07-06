#ifndef _MINISKETCH_H_
#define _MINISKETCH_H_ 1

#include <stdint.h>
#include <stdlib.h>

#ifdef _MSC_VER
#  include <BaseTsd.h>
   typedef SSIZE_T ssize_t;
#else
#  include <unistd.h>
#endif

#ifndef MINISKETCH_API
# if defined(_WIN32)
#  ifdef MINISKETCH_BUILD
#   define MINISKETCH_API __declspec(dllexport)
#  else
#   define MINISKETCH_API
#  endif
# elif defined(__GNUC__) && (__GNUC__ >= 4) && defined(MINISKETCH_BUILD)
#  define MINISKETCH_API __attribute__ ((visibility ("default")))
# else
#  define MINISKETCH_API
# endif
#endif

#ifdef __cplusplus
#  if __cplusplus >= 201103L
#    include <memory>
#    include <vector>
#    include <cassert>
#    if __cplusplus >= 201703L
#      include <optional>
#    endif
#  endif
extern "C" {
#endif

typedef struct minisketch minisketch;

MINISKETCH_API int minisketch_bits_supported(uint32_t bits);

MINISKETCH_API uint32_t minisketch_implementation_max(void);

MINISKETCH_API int minisketch_implementation_supported(uint32_t bits, uint32_t implementation);

MINISKETCH_API minisketch* minisketch_create(uint32_t bits, uint32_t implementation, size_t capacity);

MINISKETCH_API uint32_t minisketch_bits(const minisketch* sketch);

MINISKETCH_API size_t minisketch_capacity(const minisketch* sketch);

MINISKETCH_API uint32_t minisketch_implementation(const minisketch* sketch);

MINISKETCH_API void minisketch_set_seed(minisketch* sketch, uint64_t seed);

MINISKETCH_API minisketch* minisketch_clone(const minisketch* sketch);

MINISKETCH_API void minisketch_destroy(minisketch* sketch);

MINISKETCH_API size_t minisketch_serialized_size(const minisketch* sketch);

MINISKETCH_API void minisketch_serialize(const minisketch* sketch, unsigned char* output);

MINISKETCH_API void minisketch_deserialize(minisketch* sketch, const unsigned char* input);

MINISKETCH_API void minisketch_add_uint64(minisketch* sketch, uint64_t element);

MINISKETCH_API size_t minisketch_merge(minisketch* sketch, const minisketch* other_sketch);

MINISKETCH_API ssize_t minisketch_decode(const minisketch* sketch, size_t max_elements, uint64_t* output);

MINISKETCH_API size_t minisketch_compute_capacity(uint32_t bits, size_t max_elements, uint32_t fpbits);

MINISKETCH_API size_t minisketch_compute_max_elements(uint32_t bits, size_t capacity, uint32_t fpbits);

#ifdef __cplusplus
}

#if __cplusplus >= 201103L

class Minisketch
{
    struct Deleter
    {
        void operator()(minisketch* ptr) const
        {
            minisketch_destroy(ptr);
        }
    };

    std::unique_ptr<minisketch, Deleter> m_minisketch;

public:

    static bool BitsSupported(uint32_t bits) noexcept { return minisketch_bits_supported(bits); }

    static uint32_t MaxImplementation() noexcept { return minisketch_implementation_max(); }

    static bool ImplementationSupported(uint32_t bits, uint32_t implementation) noexcept { return minisketch_implementation_supported(bits, implementation); }

    static size_t ComputeCapacity(uint32_t bits, size_t max_elements, uint32_t fpbits) noexcept { return minisketch_compute_capacity(bits, max_elements, fpbits); }

    static size_t ComputeMaxElements(uint32_t bits, size_t capacity, uint32_t fpbits) noexcept { return minisketch_compute_max_elements(bits, capacity, fpbits); }

    Minisketch(const Minisketch& sketch) noexcept
    {
        if (sketch.m_minisketch) {
            m_minisketch = std::unique_ptr<minisketch, Deleter>(minisketch_clone(sketch.m_minisketch.get()));
        }
    }

    Minisketch& operator=(const Minisketch& sketch) noexcept
    {
        if (this != &sketch && sketch.m_minisketch) {
            m_minisketch = std::unique_ptr<minisketch, Deleter>(minisketch_clone(sketch.m_minisketch.get()));
        }
        return *this;
    }

    explicit operator bool() const noexcept { return bool{m_minisketch}; }

    Minisketch() noexcept = default;

    Minisketch(Minisketch&&) noexcept = default;

    Minisketch& operator=(Minisketch&&) noexcept = default;

    Minisketch(uint32_t bits, uint32_t implementation, size_t capacity) noexcept
    {
        m_minisketch = std::unique_ptr<minisketch, Deleter>(minisketch_create(bits, implementation, capacity));
    }

    static Minisketch CreateFP(uint32_t bits, uint32_t implementation, size_t max_elements, uint32_t fpbits) noexcept
    {
        return Minisketch(bits, implementation, ComputeCapacity(bits, max_elements, fpbits));
    }

    uint32_t GetBits() const noexcept { return minisketch_bits(m_minisketch.get()); }

    size_t GetCapacity() const noexcept { return minisketch_capacity(m_minisketch.get()); }

    uint32_t GetImplementation() const noexcept { return minisketch_implementation(m_minisketch.get()); }

    Minisketch& SetSeed(uint64_t seed) noexcept
    {
        minisketch_set_seed(m_minisketch.get(), seed);
        return *this;
    }

    Minisketch& Add(uint64_t element) noexcept
    {
        minisketch_add_uint64(m_minisketch.get(), element);
        return *this;
    }

    Minisketch& Merge(const Minisketch& sketch) noexcept
    {
        minisketch_merge(m_minisketch.get(), sketch.m_minisketch.get());
        return *this;
    }

    bool Decode(std::vector<uint64_t>& result) const
    {
        ssize_t ret = minisketch_decode(m_minisketch.get(), result.size(), result.data());
        if (ret == -1) return false;
        result.resize(ret);
        return true;
    }

    size_t GetSerializedSize() const noexcept { return minisketch_serialized_size(m_minisketch.get()); }

    std::vector<unsigned char> Serialize() const
    {
        std::vector<unsigned char> result(GetSerializedSize());
        minisketch_serialize(m_minisketch.get(), result.data());
        return result;
    }

    template<typename T>
    Minisketch& Deserialize(
        const T& obj,
        typename std::enable_if<
            std::is_convertible<typename std::remove_pointer<decltype(obj.data())>::type (*)[], const unsigned char (*)[]>::value &&
            std::is_convertible<decltype(obj.size()), std::size_t>::value,
            std::nullptr_t
        >::type = nullptr) noexcept
    {
        assert(GetSerializedSize() == obj.size());
        minisketch_deserialize(m_minisketch.get(), obj.data());
        return *this;
    }

#if __cplusplus >= 201703L

    std::optional<std::vector<uint64_t>> Decode(size_t max_elements) const
    {
        std::vector<uint64_t> result(max_elements);
        ssize_t ret = minisketch_decode(m_minisketch.get(), max_elements, result.data());
        if (ret == -1) return {};
        result.resize(ret);
        return result;
    }

    std::optional<std::vector<uint64_t>> DecodeFP(uint32_t fpbits) const
    {
        return Decode(ComputeMaxElements(GetBits(), GetCapacity(), fpbits));
    }
#endif
};
#endif
#endif

#endif
