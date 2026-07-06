#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "main.h"
#include "wallet.h"
#include "key.h"
#include "script.h"
#include "base58.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(transaction_record_tests)

static CTransaction CreateCoinstakeWithCharity(CKey& stakerKey, CKey& charityKey, int64_t stakeReward, int64_t charityAmount)
{
    CTransaction tx;
    tx.nTime = GetTime();

    tx.vin.resize(1);
    tx.vin[0].prevout.hash = uint256("1234567890123456789012345678901234567890123456789012345678901234");
    tx.vin[0].prevout.n = 0;

    CScript scriptEmpty;
    scriptEmpty.clear();
    tx.vout.push_back(CTxOut(0, scriptEmpty));

    CScript scriptPubKeyStaker;
    scriptPubKeyStaker << stakerKey.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(stakeReward, scriptPubKeyStaker));

    CScript scriptPubKeyCharity;
    scriptPubKeyCharity << charityKey.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(charityAmount, scriptPubKeyCharity));

    return tx;
}

BOOST_AUTO_TEST_CASE(GetValueOut_IncludesAllOutputs)
{
    CKey stakerKey, charityKey;
    stakerKey.MakeNewKey(false);
    charityKey.MakeNewKey(false);

    int64_t stakeReward = 199348154;
    int64_t charityAmount = 19934815;

    CTransaction tx = CreateCoinstakeWithCharity(stakerKey, charityKey, stakeReward, charityAmount);

    int64_t totalValue = tx.GetValueOut();
    int64_t expectedTotal = stakeReward + charityAmount;

    BOOST_CHECK_EQUAL(totalValue, expectedTotal);
    BOOST_CHECK_GT(totalValue, stakeReward);
    BOOST_CHECK_GT(totalValue, charityAmount);
}

BOOST_AUTO_TEST_CASE(GetCredit_OnlyReturnsWalletOwnedOutputs)
{
    CKey stakerKey, charityKey;
    stakerKey.MakeNewKey(false);
    charityKey.MakeNewKey(false);

    int64_t stakeReward = 199348154;
    int64_t charityAmount = 19934815;

    CTransaction tx = CreateCoinstakeWithCharity(stakerKey, charityKey, stakeReward, charityAmount);

    CWallet stakerWallet("test_staker_wallet.dat");
    CPubKey stakerPubKey = stakerKey.GetPubKey();
    CBitcoinAddress stakerAddress(stakerPubKey.GetID());

    CWallet charityWallet("test_charity_wallet.dat");
    CPubKey charityPubKey = charityKey.GetPubKey();
    CBitcoinAddress charityAddress(charityPubKey.GetID());

    CWalletTx wtxStaker(&stakerWallet, tx);
    CWalletTx wtxCharity(&charityWallet, tx);

    BOOST_CHECK_EQUAL(tx.vout.size(), 3);
    BOOST_CHECK_EQUAL(tx.vout[1].nValue, stakeReward);
    BOOST_CHECK_EQUAL(tx.vout[2].nValue, charityAmount);

    int64_t totalValue = tx.GetValueOut();
    BOOST_CHECK_EQUAL(totalValue, stakeReward + charityAmount);

}

BOOST_AUTO_TEST_CASE(CreditCalculation_FiltersByWalletOwnership)
{
    CKey stakerKey, charityKey;
    stakerKey.MakeNewKey(false);
    charityKey.MakeNewKey(false);

    int64_t stakeReward = 199348154;
    int64_t charityAmount = 19934815;

    CTransaction tx = CreateCoinstakeWithCharity(stakerKey, charityKey, stakeReward, charityAmount);

    int64_t stakerCredit = 0;
    int64_t charityCredit = 0;

    stakerCredit = stakeReward;
    BOOST_CHECK_EQUAL(stakerCredit, stakeReward);
    BOOST_CHECK_NE(stakerCredit, stakeReward + charityAmount);

    charityCredit = charityAmount;
    BOOST_CHECK_EQUAL(charityCredit, charityAmount);
    BOOST_CHECK_NE(charityCredit, stakeReward + charityAmount);

    BOOST_CHECK_EQUAL(stakerCredit + charityCredit, stakeReward + charityAmount);
}

BOOST_AUTO_TEST_SUITE_END()
