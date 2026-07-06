#include <boost/test/unit_test.hpp>
#include <limits>

#include "bignum.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(bignum_tests)

#if defined(__GNUC__)

#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else

#warning You should define NOINLINE for your compiler.
#define NOINLINE
#endif

NOINLINE void mysetint64(CBigNum& num, int64 n)
{
    num.setint64(n);
}

BOOST_AUTO_TEST_CASE(bignum_setint64)
{
    int64 n;

    {
        n = 0;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "0");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "0");
    }
    {
        n = 1;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "1");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "1");
    }
    {
        n = -1;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "-1");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "-1");
    }
    {
        n = 5;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "5");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "5");
    }
    {
        n = -5;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "-5");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "-5");
    }
    {
        n = std::numeric_limits<int64>::min();
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "-9223372036854775808");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "-9223372036854775808");
    }
    {
        n = std::numeric_limits<int64>::max();
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "9223372036854775807");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "9223372036854775807");
    }
}

BOOST_AUTO_TEST_SUITE_END()
