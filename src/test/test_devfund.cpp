#include <boost/test/unit_test.hpp>

#include "main.h"
#include "base58.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(devfund_tests)

BOOST_AUTO_TEST_CASE(test_devfund_cut_ratio)
{

    BOOST_CHECK_EQUAL(GetDevFundCut(MARYJ_STANDARD_FEE), (int64_t)(0.20 * COIN));

    BOOST_CHECK_EQUAL(GetDevFundCut(420 * COIN), 20 * COIN);

    BOOST_CHECK_EQUAL(GetDevFundCut(100 * MARYJ_STANDARD_FEE), 100 * (int64_t)(0.20 * COIN));
}

BOOST_AUTO_TEST_CASE(test_devfund_cut_edges)
{
    BOOST_CHECK_EQUAL(GetDevFundCut(0),  (int64_t)0);
    BOOST_CHECK_EQUAL(GetDevFundCut(-1), (int64_t)0);

    BOOST_CHECK_EQUAL(GetDevFundCut(20), (int64_t)0);
    BOOST_CHECK_EQUAL(GetDevFundCut(21), (int64_t)1);
}

BOOST_AUTO_TEST_CASE(test_devfund_staker_keeps_majority)
{
    int64_t nFees = 1000 * COIN;
    int64_t cut = GetDevFundCut(nFees);
    BOOST_CHECK(cut < nFees / 20);
    BOOST_CHECK(cut > nFees / 22);
    BOOST_CHECK(nFees - cut > (nFees * 95) / 100);
}

BOOST_AUTO_TEST_CASE(test_devfund_script_fail_closed_until_configured)
{
    CScript s;
    bool ok = GetDevFundScript(s);
    BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_SUITE_END()
