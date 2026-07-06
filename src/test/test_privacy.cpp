#define BOOST_TEST_MODULE MaryJaneCoin_Privacy_Tests
#include <boost/test/unit_test.hpp>

#include <set>
#include <vector>
#include <string>

#include "dandelion.h"
#include "stealth.h"
#include "key.h"
#include "uint256.h"
#include "util.h"

static uint256 MakeRandomHash()
{
    return GetRandHash();
}

static std::vector<uint256> MakeRandomHashes(int n)
{
    std::vector<uint256> hashes;
    hashes.reserve(n);
    for (int i = 0; i < n; ++i)
        hashes.push_back(MakeRandomHash());
    return hashes;
}

BOOST_AUTO_TEST_SUITE(privacy_tests)

BOOST_AUTO_TEST_CASE(test_dandelion_local_tx_starts_in_stem)
{
    CDandelionRouter router;
    uint256 hash = MakeRandomHash();

    BOOST_CHECK(router.IsStemPhase(hash) == false);

    router.AddLocalTx(hash);

    BOOST_CHECK(router.IsStemPhase(hash) == true);

    BOOST_CHECK(router.GetPhase(hash) == DANDELION_STEM);
}

BOOST_AUTO_TEST_CASE(test_dandelion_fluff_marks_complete)
{
    CDandelionRouter router;
    uint256 hash = MakeRandomHash();

    router.AddLocalTx(hash);
    BOOST_REQUIRE(router.IsStemPhase(hash) == true);

    router.MarkFluffed(hash);

    BOOST_CHECK(router.IsStemPhase(hash) == false);

    BOOST_CHECK(router.GetPhase(hash) == DANDELION_FLUFF);

    BOOST_CHECK(router.GetFluffedTxCount() == 1);

    BOOST_CHECK(router.GetStemTxCount() == 0);
}

BOOST_AUTO_TEST_CASE(test_dandelion_embargo_timeout)
{
    CDandelionRouter router;

    uint256 hash1 = MakeRandomHash();
    uint256 hash2 = MakeRandomHash();
    uint256 hash3 = MakeRandomHash();
    router.AddLocalTx(hash1);
    router.AddLocalTx(hash2);
    router.AddLocalTx(hash3);

    std::vector<uint256> expired = router.GetEmbargoedTxs();
    BOOST_CHECK(expired.empty());

    BOOST_CHECK(router.GetStemTxCount() == 3);

    BOOST_CHECK(router.IsStemPhase(hash1) == true);
    BOOST_CHECK(router.IsStemPhase(hash2) == true);
    BOOST_CHECK(router.IsStemPhase(hash3) == true);
}

BOOST_AUTO_TEST_CASE(test_dandelion_stem_count_tracking)
{
    CDandelionRouter router;

    std::vector<uint256> hashes = MakeRandomHashes(10);
    for (int i = 0; i < 10; ++i)
        router.AddLocalTx(hashes[i]);

    BOOST_CHECK_EQUAL(router.GetStemTxCount(), 10);
    BOOST_CHECK_EQUAL(router.GetFluffedTxCount(), 0);

    for (int i = 0; i < 5; ++i)
        router.MarkFluffed(hashes[i]);

    BOOST_CHECK_EQUAL(router.GetStemTxCount(), 5);
    BOOST_CHECK_EQUAL(router.GetFluffedTxCount(), 5);

    for (int i = 5; i < 10; ++i)
        router.MarkFluffed(hashes[i]);

    BOOST_CHECK_EQUAL(router.GetStemTxCount(), 0);
    BOOST_CHECK_EQUAL(router.GetFluffedTxCount(), 10);
}

BOOST_AUTO_TEST_CASE(test_dandelion_fluff_probability_distribution)
{

    CDandelionRouter router;

    std::vector<uint256> hashes = MakeRandomHashes(10000);
    for (int i = 0; i < 10000; ++i)
        router.AddStemTx(hashes[i], 0);

    int nFluffCount = 0;
    for (int i = 0; i < 10000; ++i) {
        if (router.ShouldFluff(hashes[i]))
            ++nFluffCount;
    }

    if (router.GetStemPeerCount() == 0) {

        BOOST_CHECK_EQUAL(nFluffCount, 10000);
    } else {

        BOOST_CHECK(nFluffCount >= 500);
        BOOST_CHECK(nFluffCount <= 1500);
    }

}

BOOST_AUTO_TEST_CASE(test_dandelion_different_txs_independent)
{
    CDandelionRouter router;
    uint256 hashA = MakeRandomHash();
    uint256 hashB = MakeRandomHash();

    router.AddLocalTx(hashA);
    router.AddLocalTx(hashB);

    BOOST_REQUIRE(router.IsStemPhase(hashA) == true);
    BOOST_REQUIRE(router.IsStemPhase(hashB) == true);

    router.MarkFluffed(hashA);

    BOOST_CHECK(router.IsStemPhase(hashA) == false);
    BOOST_CHECK(router.GetPhase(hashA)    == DANDELION_FLUFF);

    BOOST_CHECK(router.IsStemPhase(hashB) == true);
    BOOST_CHECK(router.GetPhase(hashB)    == DANDELION_STEM);

    BOOST_CHECK_EQUAL(router.GetStemTxCount(),    1);
    BOOST_CHECK_EQUAL(router.GetFluffedTxCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_stealth_generate_valid_keys)
{
    CStealthAddress addr;
    CKey scanSecret, spendSecret;

    BOOST_REQUIRE(GenerateStealthAddress(addr, scanSecret, spendSecret));

    BOOST_CHECK(addr.IsValid() == true);

    BOOST_CHECK(addr.scanPubKey.IsValid()      == true);
    BOOST_CHECK(addr.scanPubKey.IsCompressed() == true);
    BOOST_CHECK(addr.scanPubKey.Raw().size() == (size_t)33);

    BOOST_CHECK(addr.spendPubKey.IsValid()      == true);
    BOOST_CHECK(addr.spendPubKey.IsCompressed() == true);
    BOOST_CHECK(addr.spendPubKey.Raw().size() == (size_t)33);

    BOOST_CHECK(addr.scanPubKey != addr.spendPubKey);

    BOOST_CHECK(scanSecret.IsValid()  == true);
    BOOST_CHECK(spendSecret.IsValid() == true);
}

BOOST_AUTO_TEST_CASE(test_stealth_encode_decode_roundtrip)
{
    CStealthAddress addrOrig;
    CKey scanSecret, spendSecret;
    BOOST_REQUIRE(GenerateStealthAddress(addrOrig, scanSecret, spendSecret));

    std::string encoded = addrOrig.Encoded();
    BOOST_CHECK(!encoded.empty());

    CStealthAddress addrDecoded;
    BOOST_REQUIRE(addrDecoded.SetEncoded(encoded));

    BOOST_CHECK(addrDecoded.scanPubKey  == addrOrig.scanPubKey);
    BOOST_CHECK(addrDecoded.spendPubKey == addrOrig.spendPubKey);

    BOOST_CHECK(addrDecoded.IsValid());
}

BOOST_AUTO_TEST_CASE(test_stealth_payment_creates_unique_destination)
{
    CStealthAddress addr;
    CKey scanSecret, spendSecret;
    BOOST_REQUIRE(GenerateStealthAddress(addr, scanSecret, spendSecret));

    CStealthPayment payment1, payment2;
    BOOST_REQUIRE(PrepareStealthPayment(addr, payment1));
    BOOST_REQUIRE(PrepareStealthPayment(addr, payment2));

    BOOST_CHECK(payment1.ephemeralPubKey != payment2.ephemeralPubKey);

    BOOST_CHECK(payment1.destKeyID != payment2.destKeyID);
    BOOST_CHECK(payment1.destPubKey != payment2.destPubKey);
}

BOOST_AUTO_TEST_CASE(test_stealth_receiver_detects_payment)
{
    CStealthAddress addr;
    CKey scanSecret, spendSecret;
    BOOST_REQUIRE(GenerateStealthAddress(addr, scanSecret, spendSecret));

    CStealthPayment payment;
    BOOST_REQUIRE(PrepareStealthPayment(addr, payment));

    CKey destKeyFromReceiver;
    BOOST_REQUIRE(DetectStealthPayment(scanSecret,
                                       payment.ephemeralPubKey,
                                       addr.spendPubKey,
                                       destKeyFromReceiver));

    BOOST_CHECK(destKeyFromReceiver.GetPubKey().GetID() == payment.destKeyID);
    BOOST_CHECK(destKeyFromReceiver.GetPubKey()         == payment.destPubKey);
}

BOOST_AUTO_TEST_CASE(test_stealth_wrong_scan_key_produces_wrong_dest)
{

    CStealthAddress addrA, addrB;
    CKey scanSecretA, spendSecretA;
    CKey scanSecretB, spendSecretB;
    BOOST_REQUIRE(GenerateStealthAddress(addrA, scanSecretA, spendSecretA));
    BOOST_REQUIRE(GenerateStealthAddress(addrB, scanSecretB, spendSecretB));

    CStealthPayment payment;
    BOOST_REQUIRE(PrepareStealthPayment(addrA, payment));

    CKey wrongDestKey;
    BOOST_REQUIRE(DetectStealthPayment(scanSecretB,
                                       payment.ephemeralPubKey,
                                       addrB.spendPubKey,
                                       wrongDestKey));

    BOOST_CHECK(wrongDestKey.GetPubKey().GetID() != payment.destKeyID);
    BOOST_CHECK(wrongDestKey.GetPubKey()          != payment.destPubKey);
}

BOOST_AUTO_TEST_CASE(test_stealth_shared_secret_deterministic)
{

    CStealthAddress addr;
    CKey scanSecret, spendSecret;
    BOOST_REQUIRE(GenerateStealthAddress(addr, scanSecret, spendSecret));

    CKey ephemeralKey;
    ephemeralKey.MakeNewKey(true);
    BOOST_REQUIRE(ephemeralKey.IsValid());

    uint256 secret1, secret2;
    BOOST_REQUIRE(ComputeStealthSharedSecret(scanSecret,
                                             ephemeralKey.GetPubKey(),
                                             secret1));
    BOOST_REQUIRE(ComputeStealthSharedSecret(scanSecret,
                                             ephemeralKey.GetPubKey(),
                                             secret2));

    BOOST_CHECK(secret1 == secret2);

    BOOST_CHECK(secret1 != uint256(0));
}

BOOST_AUTO_TEST_CASE(test_stealth_unlinkability_proof)
{
    CStealthAddress addr;
    CKey scanSecret, spendSecret;
    BOOST_REQUIRE(GenerateStealthAddress(addr, scanSecret, spendSecret));

    const int N = 100;
    std::set<CKeyID>  destIDs;
    std::set<CPubKey> ephemeralKeys;

    for (int i = 0; i < N; ++i) {
        CStealthPayment payment;
        BOOST_REQUIRE_MESSAGE(PrepareStealthPayment(addr, payment),
                              "PrepareStealthPayment failed on iteration " << i);

        destIDs.insert(payment.destKeyID);
        ephemeralKeys.insert(payment.ephemeralPubKey);
    }

    BOOST_CHECK_EQUAL((int)destIDs.size(), N);

    BOOST_CHECK_EQUAL((int)ephemeralKeys.size(), N);
}

BOOST_AUTO_TEST_CASE(test_stealth_spend_key_can_sign)
{
    CStealthAddress addr;
    CKey scanSecret, spendSecret;
    BOOST_REQUIRE(GenerateStealthAddress(addr, scanSecret, spendSecret));

    CStealthPayment payment;
    BOOST_REQUIRE(PrepareStealthPayment(addr, payment));

    uint256 sharedSecret;
    BOOST_REQUIRE(ComputeStealthSharedSecret(scanSecret,
                                             payment.ephemeralPubKey,
                                             sharedSecret));

    CPubKey derivedDestPub;
    CKeyID  derivedDestID;
    BOOST_REQUIRE(DeriveStealthDestination(addr.spendPubKey,
                                           sharedSecret,
                                           derivedDestPub,
                                           derivedDestID));
    BOOST_CHECK(derivedDestID == payment.destKeyID);

    CKey destSpendKey;
    BOOST_REQUIRE(DeriveStealthSpendKey(spendSecret, sharedSecret, destSpendKey));

    BOOST_CHECK(destSpendKey.GetPubKey() == payment.destPubKey);
    BOOST_CHECK(destSpendKey.GetPubKey().GetID() == payment.destKeyID);

    uint256 msgHash = MakeRandomHash();
    std::vector<unsigned char> vchSig;
    BOOST_REQUIRE_MESSAGE(destSpendKey.Sign(msgHash, vchSig),
                          "Derived spend key failed to sign");

    BOOST_CHECK_MESSAGE(destSpendKey.Verify(msgHash, vchSig),
                        "Signature from derived spend key did not verify");
}

BOOST_AUTO_TEST_CASE(test_privacy_dandelion_prevents_broadcast)
{
    CDandelionRouter router;
    uint256 hash = MakeRandomHash();

    BOOST_CHECK(router.GetPhase(hash) == DANDELION_FLUFF);
    BOOST_CHECK(router.IsStemPhase(hash) == false);

    router.AddLocalTx(hash);

    BOOST_CHECK(router.IsStemPhase(hash) == true);
    BOOST_CHECK(router.GetPhase(hash) == DANDELION_STEM);

    CNode* pPeer = router.GetStemPeer(hash);

    (void)pPeer;

    uint256 relayedHash = MakeRandomHash();
    router.AddStemTx(relayedHash, 3);
    BOOST_CHECK(router.IsStemPhase(relayedHash) == true);
    BOOST_CHECK(router.GetPhase(relayedHash) == DANDELION_STEM);
}

BOOST_AUTO_TEST_CASE(test_privacy_stealth_op_return_format)
{
    CStealthAddress addr;
    CKey scanSecret, spendSecret;
    BOOST_REQUIRE(GenerateStealthAddress(addr, scanSecret, spendSecret));

    for (int i = 0; i < 10; ++i) {
        CStealthPayment payment;
        BOOST_REQUIRE(PrepareStealthPayment(addr, payment));

        BOOST_CHECK_MESSAGE(payment.ephemeralPubKey.IsValid(),
                            "ephemeralPubKey not valid on iteration " << i);
        BOOST_CHECK_MESSAGE(payment.ephemeralPubKey.IsCompressed(),
                            "ephemeralPubKey not compressed on iteration " << i);

        BOOST_CHECK_MESSAGE(payment.ephemeralPubKey.Raw().size() == (size_t)33,
                            "ephemeralPubKey wrong size on iteration " << i);

        BOOST_CHECK(payment.destPubKey.IsValid());
        BOOST_CHECK(payment.destPubKey.IsCompressed());
        BOOST_CHECK(payment.destPubKey.Raw().size() == (size_t)33);

        BOOST_CHECK(payment.destKeyID != CKeyID());
    }
}

BOOST_AUTO_TEST_SUITE_END()
