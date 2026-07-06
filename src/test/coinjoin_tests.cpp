#include <boost/test/unit_test.hpp>

#include "../coinjoin.h"
#include "../util.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(coinjoin_tests)

BOOST_AUTO_TEST_CASE(denom_selection_boundaries)
{

    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(3*COIN - 1), 0LL);
    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(0),          0LL);
    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(1),          0LL);

    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(3*COIN),     1*COIN);

    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(29*COIN),    1*COIN);

    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(30*COIN),    10*COIN);

    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(3000*COIN),  1000*COIN);

    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(2999*COIN),  100*COIN);

    BOOST_CHECK_EQUAL(CCoinJoinMixer::FindBestDenomination(1000000*COIN), 1000*COIN);
}

BOOST_AUTO_TEST_CASE(denom_never_exceeds_amount_over_min_outputs)
{
    const int64_t amts[] = { 3*COIN, 7*COIN, 30*COIN, 299*COIN, 3000*COIN, 12345*COIN };
    for (unsigned i = 0; i < sizeof(amts)/sizeof(amts[0]); i++)
    {
        int64_t d = CCoinJoinMixer::FindBestDenomination(amts[i]);
        BOOST_CHECK(d == 0 || d * 3 <= amts[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
