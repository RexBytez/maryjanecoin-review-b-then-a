#ifndef MARYJANECOIN_EPHEMERAL_GUARD_H
#define MARYJANECOIN_EPHEMERAL_GUARD_H

#include <stdint.h>
#include <vector>
#include <cmath>

class EphemeralKeyGuard
{
public:

    EphemeralKeyGuard(uint64_t expectedKeys = 100000, double fpRate = 1e-6)
    {
        const double LN2 = 0.6931471805599453;
        if (expectedKeys < 1) expectedKeys = 1;
        if (!(fpRate > 0.0) || fpRate >= 1.0) fpRate = 1e-6;

        double m = -((double)expectedKeys) * std::log(fpRate) / (LN2 * LN2);
        m_bits = (uint64_t)(m) + 1;
        if (m_bits < 64) m_bits = 64;
        double k = (m / (double)expectedKeys) * LN2;
        m_k = (unsigned int)(k + 0.5);
        if (m_k < 1) m_k = 1;
        if (m_k > 32) m_k = 32;
        m_data.assign((size_t)((m_bits + 7) / 8), 0);
    }

    bool WouldReuse(const unsigned char* R, size_t len) const
    {
        uint64_t h1, h2;
        hashKey(R, len, h1, h2);
        for (unsigned int i = 0; i < m_k; i++)
        {
            uint64_t bit = (h1 + (uint64_t)i * h2) % m_bits;
            if (!getBit(bit)) return false;
        }
        return true;
    }

    void Record(const unsigned char* R, size_t len)
    {
        uint64_t h1, h2;
        hashKey(R, len, h1, h2);
        for (unsigned int i = 0; i < m_k; i++)
            setBit((h1 + (uint64_t)i * h2) % m_bits);
    }

    bool WouldReuse(const std::vector<unsigned char>& R) const { return WouldReuse(R.data(), R.size()); }
    void Record(const std::vector<unsigned char>& R)          { Record(R.data(), R.size()); }

    uint64_t bits() const { return m_bits; }
    unsigned int hashes() const { return m_k; }

private:
    uint64_t m_bits;
    unsigned int m_k;
    std::vector<uint8_t> m_data;

    bool getBit(uint64_t i) const { return (m_data[(size_t)(i >> 3)] >> (i & 7)) & 1; }
    void setBit(uint64_t i)       { m_data[(size_t)(i >> 3)] |= (uint8_t)(1u << (i & 7)); }

    static void hashKey(const unsigned char* R, size_t len, uint64_t& h1, uint64_t& h2)
    {
        uint64_t a = 0xcbf29ce484222325ULL;
        for (size_t i = 0; i < len; i++) { a ^= R[i]; a *= 0x100000001b3ULL; }
        uint64_t b = a + 0x9E3779B97F4A7C15ULL;
        h1 = mix(a);
        h2 = mix(b) | 1ULL;
    }
    static uint64_t mix(uint64_t x)
    {
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
        x ^= x >> 27; x *= 0x94D049BB133111EBULL;
        x ^= x >> 31;
        return x;
    }
};

#endif
