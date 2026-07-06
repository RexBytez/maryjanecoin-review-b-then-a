#include <boost/test/unit_test.hpp>

#include <limits>
#include <vector>

#include "serialize.h"
#include "version.h"
#include "main.h"

BOOST_AUTO_TEST_SUITE(serialize_canonical_tests)

BOOST_AUTO_TEST_CASE(compactsize_canonical_roundtrip)
{
    const uint64_t cases[] = {
        0, 1, 127, 252,
        253, 254, 1000, 0xFFFF,
        0x10000, 0x123456, 0xFFFFFFFFull,
        0x100000000ull
    };
    for (uint64_t n : cases) {
        if (n > (uint64_t)MAX_SIZE) continue;
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        WriteCompactSize(ss, n);
        uint64_t got = ReadCompactSize(ss);
        BOOST_CHECK_EQUAL(got, n);
        BOOST_CHECK(ss.empty());
    }
}

BOOST_AUTO_TEST_CASE(compactsize_noncanonical_rejected)
{

    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << (unsigned char)253;
        ss << (unsigned short)5;
        BOOST_CHECK_THROW(ReadCompactSize(ss), std::ios_base::failure);
    }

    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << (unsigned char)253;
        ss << (unsigned short)252;
        BOOST_CHECK_THROW(ReadCompactSize(ss), std::ios_base::failure);
    }

    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << (unsigned char)254;
        ss << (unsigned int)0xFFFF;
        BOOST_CHECK_THROW(ReadCompactSize(ss), std::ios_base::failure);
    }

    {
        CDataStream ss(SER_DISK, PROTOCOL_VERSION);
        ss << (unsigned char)255;
        ss << (uint64_t)0xFFFFFFFFull;
        BOOST_CHECK_THROW(ReadCompactSize(ss), std::ios_base::failure);
    }
}

BOOST_AUTO_TEST_CASE(compactsize_boundary_triples)
{
    const uint64_t boundaries[] = { 252, 253, 0xFFFF, 0x10000 };
    for (uint64_t b : boundaries) {
        for (int d = -1; d <= 1; ++d) {
            uint64_t n = b + d;
            CDataStream ss(SER_DISK, PROTOCOL_VERSION);
            WriteCompactSize(ss, n);
            BOOST_CHECK_EQUAL(ReadCompactSize(ss), n);
        }
    }
}

BOOST_AUTO_TEST_CASE(compactsize_allff_too_large)
{
    CDataStream ss(SER_DISK, PROTOCOL_VERSION);
    ss << (unsigned char)255;
    ss << (uint64_t)0xFFFFFFFFFFFFFFFFull;
    BOOST_CHECK_THROW(ReadCompactSize(ss), std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(compactsize_opt_out_accepts_noncanonical)
{
    CDataStream ss(SER_DISK, PROTOCOL_VERSION);
    ss << (unsigned char)253;
    ss << (unsigned short)5;
    BOOST_CHECK_EQUAL(ReadCompactSize(ss, false), (uint64_t)5);
}

static CDataStream SerializeMinimalTx(int nVersion)
{
    CTransaction tx;
    tx.nVersion  = nVersion;
    tx.nTime     = 1000000;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.nLockTime = 0;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << tx;
    return ss;
}

BOOST_AUTO_TEST_CASE(tx_version1_roundtrips_clean)
{
    CDataStream ss = SerializeMinimalTx(1);
    CTransaction tx2;
    BOOST_CHECK_NO_THROW(ss >> tx2);
    BOOST_CHECK_EQUAL(tx2.nVersion, 1);
}

BOOST_AUTO_TEST_CASE(tx_zero_version_rejected_at_boundary)
{

    CDataStream ss = SerializeMinimalTx(1);
    ss[0] = ss[1] = ss[2] = ss[3] = (char)0x00;
    CTransaction tx2;
    BOOST_CHECK_THROW(ss >> tx2, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(tx_negative_version_rejected_at_boundary)
{

    CDataStream ss = SerializeMinimalTx(1);
    ss[0] = ss[1] = ss[2] = ss[3] = (char)0xFF;
    CTransaction tx2;
    BOOST_CHECK_THROW(ss >> tx2, std::ios_base::failure);
}

BOOST_AUTO_TEST_SUITE_END()
