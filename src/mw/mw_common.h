#ifndef MARYJANECOIN_MW_COMMON_H
#define MARYJANECOIN_MW_COMMON_H

#ifdef ENABLE_MWEB

#include "uint256.h"
#include "serialize.h"
#include "support/cleanse.h"

#include <vector>
#include <string>
#include <stdint.h>
#include <cstring>
#include <cassert>

namespace mw {

static const int MWEB_VERSION = 1;

static const size_t MAX_RANGEPROOF_SIZE     = 768;
static const size_t COMMITMENT_SIZE         = 33;
static const size_t PUBKEY_SIZE             = 33;
static const size_t SIGNATURE_SIZE          = 64;
static const size_t BLINDING_FACTOR_SIZE    = 32;
static const size_t OUTPUT_ID_SIZE          = 32;

static const uint8_t KERNEL_PLAIN          = 0x00;
static const uint8_t KERNEL_PEGIN          = 0x01;
static const uint8_t KERNEL_PEGOUT         = 0x02;
static const uint8_t KERNEL_HEIGHT_LOCKED  = 0x03;
static const uint8_t KERNEL_COINBASE       = 0x04;

static const uint8_t OUTPUT_STANDARD       = 0x00;
static const uint8_t OUTPUT_COINBASE       = 0x01;

class BlindingFactor
{
public:
    unsigned char data[BLINDING_FACTOR_SIZE];

    BlindingFactor()
    {
        memset(data, 0, BLINDING_FACTOR_SIZE);
    }

    explicit BlindingFactor(const uint256& hash)
    {
        memcpy(data, hash.begin(), BLINDING_FACTOR_SIZE);
    }

    explicit BlindingFactor(const unsigned char* pdata)
    {
        memcpy(data, pdata, BLINDING_FACTOR_SIZE);
    }

    ~BlindingFactor()
    {
        memory_cleanse(data, BLINDING_FACTOR_SIZE);
    }

    BlindingFactor(const BlindingFactor& other)
    {
        memcpy(data, other.data, BLINDING_FACTOR_SIZE);
    }

    BlindingFactor& operator=(const BlindingFactor& other)
    {
        if (this != &other)
            memcpy(data, other.data, BLINDING_FACTOR_SIZE);
        return *this;
    }

    bool IsNull() const
    {
        for (size_t i = 0; i < BLINDING_FACTOR_SIZE; i++)
            if (data[i] != 0) return false;
        return true;
    }

    uint256 GetUint256() const
    {
        uint256 result;
        memcpy((void*)result.begin(), data, BLINDING_FACTOR_SIZE);
        return result;
    }

    const unsigned char* begin() const { return data; }
    const unsigned char* end() const { return data + BLINDING_FACTOR_SIZE; }

    friend bool operator==(const BlindingFactor& a, const BlindingFactor& b)
    {
        return memcmp(a.data, b.data, BLINDING_FACTOR_SIZE) == 0;
    }

    friend bool operator!=(const BlindingFactor& a, const BlindingFactor& b)
    {
        return !(a == b);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(FLATDATA(data));
    )
};

class Commitment
{
public:
    unsigned char data[COMMITMENT_SIZE];

    Commitment()
    {
        memset(data, 0, COMMITMENT_SIZE);
    }

    explicit Commitment(const unsigned char* pdata)
    {
        memcpy(data, pdata, COMMITMENT_SIZE);
    }

    explicit Commitment(const std::vector<unsigned char>& vch)
    {
        assert(vch.size() == COMMITMENT_SIZE);
        memcpy(data, vch.data(), COMMITMENT_SIZE);
    }

    bool IsNull() const
    {
        for (size_t i = 0; i < COMMITMENT_SIZE; i++)
            if (data[i] != 0) return false;
        return true;
    }

    uint256 GetHash() const;

    std::vector<unsigned char> GetVch() const
    {
        return std::vector<unsigned char>(data, data + COMMITMENT_SIZE);
    }

    const unsigned char* begin() const { return data; }
    const unsigned char* end() const { return data + COMMITMENT_SIZE; }

    friend bool operator==(const Commitment& a, const Commitment& b)
    {
        return memcmp(a.data, b.data, COMMITMENT_SIZE) == 0;
    }

    friend bool operator!=(const Commitment& a, const Commitment& b)
    {
        return !(a == b);
    }

    friend bool operator<(const Commitment& a, const Commitment& b)
    {
        return memcmp(a.data, b.data, COMMITMENT_SIZE) < 0;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(FLATDATA(data));
    )
};

class Signature
{
public:
    unsigned char data[SIGNATURE_SIZE];

    Signature()
    {
        memset(data, 0, SIGNATURE_SIZE);
    }

    explicit Signature(const unsigned char* pdata)
    {
        memcpy(data, pdata, SIGNATURE_SIZE);
    }

    ~Signature()
    {
        memory_cleanse(data, SIGNATURE_SIZE);
    }

    Signature(const Signature& other)
    {
        memcpy(data, other.data, SIGNATURE_SIZE);
    }

    Signature& operator=(const Signature& other)
    {
        if (this != &other)
            memcpy(data, other.data, SIGNATURE_SIZE);
        return *this;
    }

    bool IsNull() const
    {
        for (size_t i = 0; i < SIGNATURE_SIZE; i++)
            if (data[i] != 0) return false;
        return true;
    }

    const unsigned char* begin() const { return data; }
    const unsigned char* end() const { return data + SIGNATURE_SIZE; }

    friend bool operator==(const Signature& a, const Signature& b)
    {
        return memcmp(a.data, b.data, SIGNATURE_SIZE) == 0;
    }

    friend bool operator!=(const Signature& a, const Signature& b)
    {
        return !(a == b);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(FLATDATA(data));
    )
};

class RangeProof
{
public:
    std::vector<unsigned char> vData;

    RangeProof() {}

    explicit RangeProof(const std::vector<unsigned char>& data) : vData(data) {}

    bool IsNull() const { return vData.empty(); }

    size_t GetSize() const { return vData.size(); }

    const unsigned char* data() const { return vData.data(); }

    IMPLEMENT_SERIALIZE(
        READWRITE(vData);
    )
};

class SecretKey
{
public:
    unsigned char data[32];

    SecretKey()
    {
        memset(data, 0, 32);
    }

    explicit SecretKey(const unsigned char* pdata)
    {
        memcpy(data, pdata, 32);
    }

    ~SecretKey()
    {
        memory_cleanse(data, 32);
    }

    SecretKey(const SecretKey& other)
    {
        memcpy(data, other.data, 32);
    }

    SecretKey& operator=(const SecretKey& other)
    {
        if (this != &other)
            memcpy(data, other.data, 32);
        return *this;
    }

    bool IsNull() const
    {
        for (size_t i = 0; i < 32; i++)
            if (data[i] != 0) return false;
        return true;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(FLATDATA(data));
    )
};

}

#endif
#endif
