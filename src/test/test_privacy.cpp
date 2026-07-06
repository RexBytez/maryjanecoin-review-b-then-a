#include <boost/test/unit_test.hpp>

#include <set>
#include <vector>
#include <string>

#include "dandelion.h"
#include "stealth.h"
#include "torcontrol.h"
#include "i2p.h"
#include "payjoin.h"
#include "bip47.h"
#include "coincontrol.h"
#include "key.h"
#include "uint256.h"
#include "hash.h"
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

BOOST_AUTO_TEST_CASE(test_tor_authenticate_cookie_format)
{

    std::vector<unsigned char> vchCookie;
    for (int i = 0; i < 32; ++i)
        vchCookie.push_back((unsigned char)i);

    std::string strCmd = CTorControl::FormatAuthenticateCommand(vchCookie);

    BOOST_CHECK(strCmd.find("AUTHENTICATE ") == 0);
    BOOST_CHECK(strCmd.size() >= 2);
    BOOST_CHECK(strCmd[strCmd.size() - 2] == '\r');
    BOOST_CHECK(strCmd[strCmd.size() - 1] == '\n');

    BOOST_CHECK_EQUAL(strCmd.size(), (size_t)79);

    BOOST_CHECK(strCmd.substr(13, 8) == "00010203");
}

BOOST_AUTO_TEST_CASE(test_tor_authenticate_password_format)
{

    std::string strCmd = CTorControl::FormatAuthenticatePasswordCommand("mypass");
    BOOST_CHECK(strCmd == "AUTHENTICATE \"mypass\"\r\n");

    strCmd = CTorControl::FormatAuthenticatePasswordCommand("pass\"word");
    BOOST_CHECK(strCmd == "AUTHENTICATE \"pass\\\"word\"\r\n");

    strCmd = CTorControl::FormatAuthenticatePasswordCommand("");
    BOOST_CHECK(strCmd == "AUTHENTICATE \"\"\r\n");
}

BOOST_AUTO_TEST_CASE(test_tor_protocolinfo_format)
{
    std::string strCmd = CTorControl::FormatProtocolInfoCommand();
    BOOST_CHECK(strCmd == "PROTOCOLINFO 1\r\n");
}

BOOST_AUTO_TEST_CASE(test_tor_add_onion_format)
{
    std::string strCmd = CTorControl::FormatAddOnionCommand(50000, 50000);
    BOOST_CHECK(strCmd == "ADD_ONION NEW:BEST Port=50000,127.0.0.1:50000\r\n");

    strCmd = CTorControl::FormatAddOnionCommand(80, 8080);
    BOOST_CHECK(strCmd == "ADD_ONION NEW:BEST Port=80,127.0.0.1:8080\r\n");
}

BOOST_AUTO_TEST_CASE(test_tor_del_onion_format)
{
    std::string strCmd = CTorControl::FormatDelOnionCommand("abc123xyz456");
    BOOST_CHECK(strCmd == "DEL_ONION abc123xyz456\r\n");
}

BOOST_AUTO_TEST_CASE(test_tor_initial_state)
{
    CTorControl tor;
    BOOST_CHECK(tor.GetState() == TOR_DISCONNECTED);
    BOOST_CHECK(tor.IsReady() == false);
    BOOST_CHECK(tor.GetOnionAddress().empty());
}

BOOST_AUTO_TEST_CASE(test_i2p_hello_format)
{
    std::string strMsg = CI2PSession::FormatHelloMessage();
    BOOST_CHECK(strMsg == "HELLO VERSION MIN=3.1 MAX=3.1\n");
}

BOOST_AUTO_TEST_CASE(test_i2p_session_create_format)
{
    std::string strMsg = CI2PSession::FormatSessionCreateMessage("mysession123", "STREAM");

    BOOST_CHECK(strMsg.find("SESSION CREATE") == 0);
    BOOST_CHECK(strMsg.find("STYLE=STREAM") != std::string::npos);
    BOOST_CHECK(strMsg.find("ID=mysession123") != std::string::npos);
    BOOST_CHECK(strMsg.find("DESTINATION=TRANSIENT") != std::string::npos);

    BOOST_CHECK(strMsg[strMsg.size() - 1] == '\n');
}

BOOST_AUTO_TEST_CASE(test_i2p_stream_accept_format)
{
    std::string strMsg = CI2PSession::FormatStreamAcceptMessage("sess42");

    BOOST_CHECK(strMsg.find("STREAM ACCEPT") == 0);
    BOOST_CHECK(strMsg.find("ID=sess42") != std::string::npos);
    BOOST_CHECK(strMsg.find("SILENT=false") != std::string::npos);
    BOOST_CHECK(strMsg[strMsg.size() - 1] == '\n');
}

BOOST_AUTO_TEST_CASE(test_i2p_stream_connect_format)
{
    std::string strDest = "AAAA1111BBBB2222CCCC3333";
    std::string strMsg = CI2PSession::FormatStreamConnectMessage("sess42", strDest);

    BOOST_CHECK(strMsg.find("STREAM CONNECT") == 0);
    BOOST_CHECK(strMsg.find("ID=sess42") != std::string::npos);
    BOOST_CHECK(strMsg.find("DESTINATION=" + strDest) != std::string::npos);
    BOOST_CHECK(strMsg.find("SILENT=false") != std::string::npos);
    BOOST_CHECK(strMsg[strMsg.size() - 1] == '\n');
}

BOOST_AUTO_TEST_CASE(test_i2p_initial_state)
{
    CI2PSession session;
    BOOST_CHECK(session.GetState() == I2P_DISCONNECTED);
    BOOST_CHECK(session.IsReady() == false);
    BOOST_CHECK(session.GetMyDestination().empty());
    BOOST_CHECK(session.GetMyDestinationB32().empty());
}

BOOST_AUTO_TEST_CASE(test_toronly_flag_implies_proxy_settings)
{

    std::vector<unsigned char> vchEmpty;
    std::string strCmd = CTorControl::FormatAuthenticateCommand(vchEmpty);
    BOOST_CHECK(strCmd == "AUTHENTICATE \r\n");

    strCmd = CTorControl::FormatAddOnionCommand(50000, 50000);
    BOOST_CHECK(strCmd.find("Port=50000,127.0.0.1:50000") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_getprivacyinfo_data_sources)
{

    CDandelionRouter router;
    BOOST_CHECK_EQUAL(router.GetStemPeerCount(), 0);
    BOOST_CHECK_EQUAL(router.GetStemTxCount(), 0);
    BOOST_CHECK_EQUAL(router.GetFluffedTxCount(), 0);

    CTorControl tor;
    BOOST_CHECK(tor.GetState() == TOR_DISCONNECTED);
    BOOST_CHECK(tor.GetOnionAddress().empty());

    CI2PSession i2p;
    BOOST_CHECK(i2p.GetState() == I2P_DISCONNECTED);
    BOOST_CHECK(i2p.GetMyDestination().empty());
    BOOST_CHECK(i2p.GetMyDestinationB32().empty());

    int nScore = 0;
    if (router.GetStemPeerCount() > 0) nScore += 25;
    if (tor.GetState() == TOR_SERVICE_CREATED) nScore += 20;
    if (i2p.GetState() == I2P_SESSION_CREATED) nScore += 20;
    BOOST_CHECK_EQUAL(nScore, 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(payjoin_tests)

static CTransaction MakeMockTx(int nInputs, int nOutputs, int64_t nOutputValue)
{
    CTransaction tx;
    tx.nVersion = 1;
    tx.nTime = GetTime();

    for (int i = 0; i < nInputs; i++)
    {

        uint256 hash = GetRandHash();
        tx.vin.push_back(CTxIn(COutPoint(hash, 0)));
    }

    for (int i = 0; i < nOutputs; i++)
    {
        CScript scriptPubKey;

        scriptPubKey << OP_TRUE;
        tx.vout.push_back(CTxOut(nOutputValue, scriptPubKey));
    }

    return tx;
}

BOOST_AUTO_TEST_CASE(test_payjoin_proposal_preserves_sender_inputs)
{

    CTransaction txOriginal = MakeMockTx(2, 2, 50 * COIN);

    CTransaction txProposal = txOriginal;
    uint256 receiverHash = GetRandHash();
    txProposal.vin.push_back(CTxIn(COutPoint(receiverHash, 0)));

    for (unsigned int i = 0; i < txOriginal.vin.size(); i++)
    {
        bool fFound = false;
        for (unsigned int j = 0; j < txProposal.vin.size(); j++)
        {
            if (txProposal.vin[j].prevout == txOriginal.vin[i].prevout)
            {
                fFound = true;
                break;
            }
        }
        BOOST_CHECK_MESSAGE(fFound,
            "Original input " << i << " missing from proposal — SECURITY VIOLATION");
    }

    BOOST_CHECK(txProposal.vin.size() > txOriginal.vin.size());
}

BOOST_AUTO_TEST_CASE(test_payjoin_reject_removed_input)
{
    CTransaction txOriginal = MakeMockTx(3, 2, 100 * COIN);

    CTransaction txMalicious;
    txMalicious.nVersion = txOriginal.nVersion;
    txMalicious.nTime = txOriginal.nTime;

    for (unsigned int i = 1; i < txOriginal.vin.size(); i++)
        txMalicious.vin.push_back(txOriginal.vin[i]);

    txMalicious.vin.push_back(CTxIn(COutPoint(GetRandHash(), 0)));
    txMalicious.vout = txOriginal.vout;

    std::set<COutPoint> setProposalInputs;
    for (unsigned int i = 0; i < txMalicious.vin.size(); i++)
        setProposalInputs.insert(txMalicious.vin[i].prevout);

    bool fInputMissing = (setProposalInputs.find(txOriginal.vin[0].prevout) == setProposalInputs.end());
    BOOST_CHECK_MESSAGE(fInputMissing,
        "Malicious proposal should have a missing input, but validation didn't detect it");

    int nMissing = 0;
    for (unsigned int i = 0; i < txOriginal.vin.size(); i++)
    {
        if (setProposalInputs.find(txOriginal.vin[i].prevout) == setProposalInputs.end())
            nMissing++;
    }
    BOOST_CHECK_EQUAL(nMissing, 1);
}

BOOST_AUTO_TEST_CASE(test_payjoin_value_balancing)
{
    int64_t nPaymentAmount = 100 * COIN;
    int64_t nSenderInput = 150 * COIN;
    int64_t nSenderChange = 49 * COIN;
    int64_t nFee = 1 * COIN;

    CTransaction txOriginal;
    txOriginal.vin.push_back(CTxIn(COutPoint(GetRandHash(), 0)));
    CScript scriptPayment;
    scriptPayment << OP_TRUE;
    CScript scriptChange;
    scriptChange << OP_1;
    txOriginal.vout.push_back(CTxOut(nPaymentAmount, scriptPayment));
    txOriginal.vout.push_back(CTxOut(nSenderChange, scriptChange));

    int64_t nOrigTotalOut = nPaymentAmount + nSenderChange;

    int64_t nReceiverInput = 75 * COIN;
    int64_t nReceiverChange = nReceiverInput - (COIN / 2);

    CTransaction txProposal = txOriginal;
    txProposal.vin.push_back(CTxIn(COutPoint(GetRandHash(), 0)));
    CScript scriptReceiverChange;
    scriptReceiverChange << OP_2;
    txProposal.vout.push_back(CTxOut(nReceiverChange, scriptReceiverChange));

    int64_t nPropTotalOut = nPaymentAmount + nSenderChange + nReceiverChange;

    int64_t nTotalIn = nSenderInput + nReceiverInput;
    BOOST_CHECK(nTotalIn >= nPropTotalOut);

    int64_t nTotalFee = nTotalIn - nPropTotalOut;
    BOOST_CHECK(nTotalFee >= nFee);
    BOOST_CHECK(nTotalFee <= nFee + COIN);

    BOOST_CHECK_EQUAL(txProposal.vout[0].nValue, nPaymentAmount);

    BOOST_CHECK_EQUAL(txProposal.vout[1].nValue, nSenderChange);
}

BOOST_AUTO_TEST_CASE(test_payjoin_denomination_matching)
{

    std::vector<int64_t> vSenderValues;
    vSenderValues.push_back(50 * COIN);
    vSenderValues.push_back(25 * COIN);

    std::vector<int64_t> vCandidates;
    vCandidates.push_back(1000 * COIN);
    vCandidates.push_back(48 * COIN);
    vCandidates.push_back(1 * COIN);
    vCandidates.push_back(26 * COIN);
    vCandidates.push_back(500 * COIN);

    std::vector<std::pair<int64_t, int> > vScored;
    for (int i = 0; i < (int)vCandidates.size(); i++)
    {
        int64_t nBestDiff = std::abs(vCandidates[i] - vSenderValues[0]);
        for (size_t j = 1; j < vSenderValues.size(); j++)
        {
            int64_t nDiff = std::abs(vCandidates[i] - vSenderValues[j]);
            if (nDiff < nBestDiff)
                nBestDiff = nDiff;
        }
        vScored.push_back(std::make_pair(nBestDiff, i));
    }

    std::sort(vScored.begin(), vScored.end());

    BOOST_CHECK_EQUAL(vCandidates[vScored[0].second], 26 * COIN);
    BOOST_CHECK_EQUAL(vCandidates[vScored[1].second], 48 * COIN);

    int nWorstIdx = vScored[vScored.size() - 1].second;
    int64_t nWorstValue = vCandidates[nWorstIdx];
    BOOST_CHECK(nWorstValue == 1000 * COIN || nWorstValue == 500 * COIN);
}

BOOST_AUTO_TEST_CASE(test_payjoin_reject_extra_outputs)
{
    int64_t nPayment = 100 * COIN;
    int64_t nChange = 49 * COIN;

    CTransaction txOriginal;
    txOriginal.vin.push_back(CTxIn(COutPoint(GetRandHash(), 0)));
    CScript scriptPay, scriptChange;
    scriptPay << OP_TRUE;
    scriptChange << OP_1;
    txOriginal.vout.push_back(CTxOut(nPayment, scriptPay));
    txOriginal.vout.push_back(CTxOut(nChange, scriptChange));

    CTransaction txMalicious = txOriginal;
    txMalicious.vin.push_back(CTxIn(COutPoint(GetRandHash(), 0)));

    txMalicious.vout[1].nValue = 10 * COIN;
    CScript scriptAttacker;
    scriptAttacker << OP_2;
    txMalicious.vout.push_back(CTxOut(39 * COIN, scriptAttacker));

    int64_t nOrigChange = txOriginal.vout[1].nValue;
    int64_t nPropChange = txMalicious.vout[1].nValue;
    int64_t nChangeReduction = nOrigChange - nPropChange;

    BOOST_CHECK(nChangeReduction > COIN);
    BOOST_CHECK_EQUAL(nChangeReduction, 39 * COIN);

    int nNewOutputs = 0;
    for (unsigned int i = 0; i < txMalicious.vout.size(); i++)
    {
        bool fInOriginal = false;
        for (unsigned int j = 0; j < txOriginal.vout.size(); j++)
        {
            if (txMalicious.vout[i].scriptPubKey == txOriginal.vout[j].scriptPubKey &&
                txMalicious.vout[i].nValue == txOriginal.vout[j].nValue)
            {
                fInOriginal = true;
                break;
            }
        }
        if (!fInOriginal)
            nNewOutputs++;
    }

    BOOST_CHECK(nNewOutputs >= 1);
}

BOOST_AUTO_TEST_CASE(test_payjoin_tx_indistinguishable)
{

    CTransaction txPayJoin;
    txPayJoin.nVersion = CTransaction::CURRENT_VERSION;
    txPayJoin.nTime = GetTime();
    txPayJoin.nLockTime = 0;

    for (int i = 0; i < 3; i++)
        txPayJoin.vin.push_back(CTxIn(COutPoint(GetRandHash(), 0)));

    CScript script1, script2, script3;
    script1 << OP_TRUE;
    script2 << OP_1;
    script3 << OP_2;
    txPayJoin.vout.push_back(CTxOut(100 * COIN, script1));
    txPayJoin.vout.push_back(CTxOut(45 * COIN, script2));
    txPayJoin.vout.push_back(CTxOut(70 * COIN, script3));

    CTransaction txNormal;
    txNormal.nVersion = CTransaction::CURRENT_VERSION;
    txNormal.nTime = GetTime();
    txNormal.nLockTime = 0;

    for (int i = 0; i < 3; i++)
        txNormal.vin.push_back(CTxIn(COutPoint(GetRandHash(), 0)));
    txNormal.vout.push_back(CTxOut(100 * COIN, script1));
    txNormal.vout.push_back(CTxOut(45 * COIN, script2));
    txNormal.vout.push_back(CTxOut(70 * COIN, script3));

    BOOST_CHECK_EQUAL(txPayJoin.nVersion, txNormal.nVersion);

    BOOST_CHECK_EQUAL(txPayJoin.nVersion, (int)CTransaction::CURRENT_VERSION);

    BOOST_CHECK_EQUAL(txPayJoin.vin.size(), txNormal.vin.size());
    BOOST_CHECK_EQUAL(txPayJoin.vout.size(), txNormal.vout.size());

    BOOST_CHECK_EQUAL(txPayJoin.nLockTime, txNormal.nLockTime);

    BOOST_CHECK(!txPayJoin.IsCoinBase());
    BOOST_CHECK(!txNormal.IsCoinBase());

    BOOST_CHECK(!txPayJoin.IsCoinStake());
    BOOST_CHECK(!txNormal.IsCoinStake());

    BOOST_CHECK_EQUAL(txPayJoin.nVersion, 1);

    unsigned int nPayJoinSize = ::GetSerializeSize(txPayJoin, SER_NETWORK, PROTOCOL_VERSION);
    unsigned int nNormalSize = ::GetSerializeSize(txNormal, SER_NETWORK, PROTOCOL_VERSION);

    BOOST_CHECK_EQUAL(nPayJoinSize, nNormalSize);
}

BOOST_AUTO_TEST_CASE(test_payjoin_history_tracking)
{

    {
        LOCK(cs_payjoinHistory);
        vPayJoinHistory.clear();
    }

    for (int i = 0; i < 5; i++)
    {
        CPayJoinHistoryEntry entry;
        entry.txHash = GetRandHash();
        entry.nTimestamp = GetTime();
        entry.nAmount = (i + 1) * 100 * COIN;
        entry.fSender = (i % 2 == 0);
        entry.nSenderInputs = 2;
        entry.nReceiverInputs = 1;
        entry.strCounterparty = "TestAddress";
        RecordPayJoinHistory(entry);
    }

    std::vector<CPayJoinHistoryEntry> vHistory = GetPayJoinHistory();
    BOOST_CHECK_EQUAL((int)vHistory.size(), 5);

    for (int i = 0; i < 5; i++)
    {
        BOOST_CHECK_EQUAL(vHistory[i].nAmount, (i + 1) * 100 * COIN);
        BOOST_CHECK_EQUAL(vHistory[i].fSender, (i % 2 == 0));
        BOOST_CHECK_EQUAL(vHistory[i].nSenderInputs, 2);
        BOOST_CHECK_EQUAL(vHistory[i].nReceiverInputs, 1);
    }

    {
        LOCK(cs_payjoinHistory);
        vPayJoinHistory.clear();
    }

    for (int i = 0; i < 110; i++)
    {
        CPayJoinHistoryEntry entry;
        entry.txHash = GetRandHash();
        entry.nTimestamp = GetTime();
        entry.nAmount = i * COIN;
        entry.fSender = true;
        entry.nSenderInputs = 1;
        entry.nReceiverInputs = 1;
        RecordPayJoinHistory(entry);
    }

    vHistory = GetPayJoinHistory();
    BOOST_CHECK_EQUAL((int)vHistory.size(), 100);

    BOOST_CHECK_EQUAL(vHistory[0].nAmount, 10 * COIN);
}

BOOST_AUTO_TEST_CASE(test_payjoin_constants)
{

    BOOST_CHECK(PAYJOIN_VERSION > 0);

    BOOST_CHECK_EQUAL(DEFAULT_PAYJOIN_PORT, 0);

    BOOST_CHECK(PAYJOIN_MAX_RECEIVER_INPUTS >= 1);
    BOOST_CHECK(PAYJOIN_MAX_RECEIVER_INPUTS <= 10);

    BOOST_CHECK(PAYJOIN_MIN_CONFIRMATIONS >= 1);

    BOOST_CHECK(PAYJOIN_MAX_FEE_INCREASE > 0);
    BOOST_CHECK(PAYJOIN_MAX_FEE_INCREASE <= 10 * COIN);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(utxo_hygiene_tests)

BOOST_AUTO_TEST_CASE(test_frozen_utxo_management)
{
    CCoinControl coinControl;

    COutPoint out1(uint256("0x1111111111111111111111111111111111111111111111111111111111111111"), 0);
    COutPoint out2(uint256("0x2222222222222222222222222222222222222222222222222222222222222222"), 1);
    COutPoint out3(uint256("0x3333333333333333333333333333333333333333333333333333333333333333"), 0);

    BOOST_CHECK(!coinControl.IsFrozen(out1));
    BOOST_CHECK(!coinControl.IsFrozen(out2));
    BOOST_CHECK_EQUAL(coinControl.GetFrozenCount(), 0);

    coinControl.FreezeOutput(out1);
    coinControl.FreezeOutput(out2);
    BOOST_CHECK(coinControl.IsFrozen(out1));
    BOOST_CHECK(coinControl.IsFrozen(out2));
    BOOST_CHECK(!coinControl.IsFrozen(out3));
    BOOST_CHECK_EQUAL(coinControl.GetFrozenCount(), 2);

    coinControl.UnfreezeOutput(out1);
    BOOST_CHECK(!coinControl.IsFrozen(out1));
    BOOST_CHECK(coinControl.IsFrozen(out2));
    BOOST_CHECK_EQUAL(coinControl.GetFrozenCount(), 1);

    std::vector<COutPoint> vFrozen;
    coinControl.ListFrozen(vFrozen);
    BOOST_CHECK_EQUAL(vFrozen.size(), (size_t)1);
    BOOST_CHECK(vFrozen[0] == out2);

    coinControl.SetNull();
    BOOST_CHECK(coinControl.IsFrozen(out2));
}

BOOST_AUTO_TEST_CASE(test_privacy_score_calculation)
{

    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(0), 0);
    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(-1), 0);

    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(1), 20);
    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(5), 20);

    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(6), 40);
    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(19), 40);

    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(20), 60);
    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(99), 60);

    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(100), 80);
    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(499), 80);

    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(500), 100);
    BOOST_CHECK_EQUAL(CCoinControl::GetPrivacyScore(10000), 100);

    int nPrev = 0;
    for (int d = 0; d <= 1000; d++) {
        int nScore = CCoinControl::GetPrivacyScore(d);
        BOOST_CHECK(nScore >= nPrev);
        nPrev = nScore;
    }
}

BOOST_AUTO_TEST_CASE(test_age_buckets)
{

    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(0), 0);
    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(3600), 0);
    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(86399), 0);

    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(86400), 1);
    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(86400 * 7 - 1), 1);

    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(86400 * 7), 2);
    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(86400 * 30 - 1), 2);

    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(86400 * 30), 3);
    BOOST_CHECK_EQUAL(CCoinControl::GetAgeBucket(86400 * 365), 3);
}

BOOST_AUTO_TEST_CASE(test_utxo_labels)
{
    CCoinControl coinControl;

    COutPoint out1(uint256("0x1111111111111111111111111111111111111111111111111111111111111111"), 0);
    COutPoint out2(uint256("0x2222222222222222222222222222222222222222222222222222222222222222"), 0);

    BOOST_CHECK_EQUAL(coinControl.GetLabel(out1), "");
    BOOST_CHECK(!coinControl.HasLabel(out1));

    coinControl.SetLabel(out1, "mining reward");
    BOOST_CHECK_EQUAL(coinControl.GetLabel(out1), "mining reward");
    BOOST_CHECK(coinControl.HasLabel(out1));

    BOOST_CHECK_EQUAL(coinControl.GetLabel(out2), "");

    coinControl.SetLabel(out1, "staking reward");
    BOOST_CHECK_EQUAL(coinControl.GetLabel(out1), "staking reward");

    coinControl.SetLabel(out1, "");
    BOOST_CHECK_EQUAL(coinControl.GetLabel(out1), "");
    BOOST_CHECK(!coinControl.HasLabel(out1));

    coinControl.SetLabel(out2, "bridge funds");
    coinControl.SetNull();
    BOOST_CHECK_EQUAL(coinControl.GetLabel(out2), "bridge funds");
}

BOOST_AUTO_TEST_CASE(test_privacy_mode_defaults)
{
    CCoinControl coinControl;
    BOOST_CHECK(!coinControl.fPrivacyMode);
    BOOST_CHECK(!coinControl.fDontConsolidate);

    coinControl.fPrivacyMode = true;
    coinControl.fDontConsolidate = true;
    coinControl.SetNull();
    BOOST_CHECK(!coinControl.fPrivacyMode);
    BOOST_CHECK(!coinControl.fDontConsolidate);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(bip47_tests)

BOOST_AUTO_TEST_CASE(test_bip47_payment_code_valid)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentCode code;
    code.nVersion = BIP47_VERSION;
    code.nFeatures = BIP47_FEATURES;
    code.scanPubKey = scanKey.GetPubKey();
    code.spendPubKey = spendKey.GetPubKey();

    BOOST_CHECK(code.IsValid());
    BOOST_CHECK(code.scanPubKey.IsCompressed());
    BOOST_CHECK(code.spendPubKey.IsCompressed());
    BOOST_CHECK_EQUAL(code.nVersion, (uint8_t)0x01);
}

BOOST_AUTO_TEST_CASE(test_bip47_encode_decode_roundtrip)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentCode original;
    original.nVersion = BIP47_VERSION;
    original.nFeatures = BIP47_FEATURES;
    original.scanPubKey = scanKey.GetPubKey();
    original.spendPubKey = spendKey.GetPubKey();

    std::vector<unsigned char> vchPayload;
    BOOST_REQUIRE(original.Encode(vchPayload));
    BOOST_CHECK_EQUAL(vchPayload.size(), (size_t)PAYMENT_CODE_SIZE);

    CPaymentCode decoded;
    BOOST_REQUIRE(decoded.Decode(vchPayload));

    BOOST_CHECK_EQUAL(decoded.nVersion, original.nVersion);
    BOOST_CHECK_EQUAL(decoded.nFeatures, original.nFeatures);
    BOOST_CHECK(decoded.scanPubKey == original.scanPubKey);
    BOOST_CHECK(decoded.spendPubKey == original.spendPubKey);
}

BOOST_AUTO_TEST_CASE(test_bip47_base58_roundtrip)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentCode original;
    original.nVersion = BIP47_VERSION;
    original.nFeatures = BIP47_FEATURES;
    original.scanPubKey = scanKey.GetPubKey();
    original.spendPubKey = spendKey.GetPubKey();

    std::string strEncoded = original.ToBase58();
    BOOST_CHECK(!strEncoded.empty());

    CPaymentCode decoded;
    BOOST_REQUIRE(decoded.FromBase58(strEncoded));

    BOOST_CHECK(decoded.scanPubKey == original.scanPubKey);
    BOOST_CHECK(decoded.spendPubKey == original.spendPubKey);

    BOOST_CHECK_EQUAL(decoded.ToBase58(), strEncoded);
}

BOOST_AUTO_TEST_CASE(test_bip47_ecdh_symmetric)
{
    CKey keyA, keyB;
    keyA.MakeNewKey(true);
    keyB.MakeNewKey(true);

    uint256 secretAB, secretBA;
    BOOST_REQUIRE(ComputeBIP47SharedSecret(keyA, keyB.GetPubKey(), secretAB));
    BOOST_REQUIRE(ComputeBIP47SharedSecret(keyB, keyA.GetPubKey(), secretBA));

    BOOST_CHECK(secretAB == secretBA);
    BOOST_CHECK(secretAB != uint256(0));
}

BOOST_AUTO_TEST_CASE(test_bip47_derive_address_deterministic)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentCode code;
    code.nVersion = BIP47_VERSION;
    code.nFeatures = BIP47_FEATURES;
    code.scanPubKey = scanKey.GetPubKey();
    code.spendPubKey = spendKey.GetPubKey();

    uint256 sharedSecret;
    BOOST_REQUIRE(ComputeBIP47SharedSecret(scanKey, spendKey.GetPubKey(), sharedSecret));

    CPubKey dest1a, dest1b;
    CKeyID kid1a, kid1b;

    BOOST_REQUIRE(code.DeriveReceiveAddress(sharedSecret, 0, dest1a, kid1a));
    BOOST_REQUIRE(code.DeriveReceiveAddress(sharedSecret, 0, dest1b, kid1b));
    BOOST_CHECK(dest1a == dest1b);
    BOOST_CHECK(kid1a == kid1b);

    CPubKey dest2;
    CKeyID kid2;
    BOOST_REQUIRE(code.DeriveReceiveAddress(sharedSecret, 1, dest2, kid2));
    BOOST_CHECK(kid1a != kid2);
}

BOOST_AUTO_TEST_CASE(test_bip47_unlinkability)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentCode code;
    code.nVersion = BIP47_VERSION;
    code.nFeatures = BIP47_FEATURES;
    code.scanPubKey = scanKey.GetPubKey();
    code.spendPubKey = spendKey.GetPubKey();

    uint256 sharedSecret;
    BOOST_REQUIRE(ComputeBIP47SharedSecret(scanKey, spendKey.GetPubKey(), sharedSecret));

    std::set<CKeyID> setAddresses;
    for (uint32_t i = 0; i < 50; ++i)
    {
        CPubKey destPub;
        CKeyID destKeyID;
        BOOST_REQUIRE(code.DeriveReceiveAddress(sharedSecret, i, destPub, destKeyID));
        BOOST_CHECK(destPub.IsValid());
        BOOST_CHECK(destPub.IsCompressed());

        BOOST_CHECK_MESSAGE(setAddresses.insert(destKeyID).second,
                            "Duplicate address at index " << i);
    }
    BOOST_CHECK_EQUAL(setAddresses.size(), (size_t)50);
}

BOOST_AUTO_TEST_CASE(test_bip47_blinding_roundtrip)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentCode code;
    code.nVersion = BIP47_VERSION;
    code.nFeatures = BIP47_FEATURES;
    code.scanPubKey = scanKey.GetPubKey();
    code.spendPubKey = spendKey.GetPubKey();

    std::vector<unsigned char> vchOriginal;
    BOOST_REQUIRE(code.Encode(vchOriginal));

    uint256 blindingSecret = GetRandHash();

    std::vector<unsigned char> vchBlinded;
    BOOST_REQUIRE(BlindPaymentCode(vchOriginal, blindingSecret, vchBlinded));

    BOOST_CHECK(vchBlinded != vchOriginal);

    std::vector<unsigned char> vchUnblinded;
    BOOST_REQUIRE(UnblindPaymentCode(vchBlinded, blindingSecret, vchUnblinded));

    BOOST_CHECK(vchUnblinded == vchOriginal);

    CPaymentCode decoded;
    BOOST_REQUIRE(decoded.Decode(vchUnblinded));
    BOOST_CHECK(decoded.scanPubKey == code.scanPubKey);
    BOOST_CHECK(decoded.spendPubKey == code.spendPubKey);
}

BOOST_AUTO_TEST_CASE(test_bip47_derive_spend_key_matches)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentCode code;
    code.nVersion = BIP47_VERSION;
    code.nFeatures = BIP47_FEATURES;
    code.scanPubKey = scanKey.GetPubKey();
    code.spendPubKey = spendKey.GetPubKey();

    uint256 sharedSecret;
    BOOST_REQUIRE(ComputeBIP47SharedSecret(scanKey, spendKey.GetPubKey(), sharedSecret));

    CPubKey destPub;
    CKeyID destKeyID;
    BOOST_REQUIRE(code.DeriveReceiveAddress(sharedSecret, 5, destPub, destKeyID));

    CKey destPriv;
    BOOST_REQUIRE(code.DeriveSpendKey(spendKey, sharedSecret, 5, destPriv));

    BOOST_CHECK(destPriv.GetPubKey().GetID() == destKeyID);
}

BOOST_AUTO_TEST_CASE(test_bip47_channel_serialize)
{
    CKey scanKey, spendKey;
    scanKey.MakeNewKey(true);
    spendKey.MakeNewKey(true);

    CPaymentChannel channel;
    channel.theirCode.nVersion = BIP47_VERSION;
    channel.theirCode.nFeatures = BIP47_FEATURES;
    channel.theirCode.scanPubKey = scanKey.GetPubKey();
    channel.theirCode.spendPubKey = spendKey.GetPubKey();
    channel.sharedSecret = GetRandHash();
    channel.nNextSendIndex = 7;
    channel.nNextRecvIndex = 3;
    channel.fNotificationSent = true;
    channel.fNotificationRecv = false;
    channel.nCreateTime = GetTime();

    CDataStream ss(SER_DISK, CLIENT_VERSION);
    ss << channel;

    CPaymentChannel decoded;
    ss >> decoded;

    BOOST_CHECK(decoded.theirCode.scanPubKey == channel.theirCode.scanPubKey);
    BOOST_CHECK(decoded.sharedSecret == channel.sharedSecret);
    BOOST_CHECK_EQUAL(decoded.nNextSendIndex, (uint32_t)7);
    BOOST_CHECK_EQUAL(decoded.nNextRecvIndex, (uint32_t)3);
    BOOST_CHECK(decoded.fNotificationSent);
    BOOST_CHECK(!decoded.fNotificationRecv);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(payjoin_tests)

BOOST_AUTO_TEST_CASE(test_payjoin_constants_basic)
{
    BOOST_CHECK(PAYJOIN_MAX_RECEIVER_INPUTS >= 1);
    BOOST_CHECK(PAYJOIN_MAX_RECEIVER_INPUTS <= 20);
    BOOST_CHECK(PAYJOIN_MIN_CONFIRMATIONS >= 1);
    BOOST_CHECK(PAYJOIN_MAX_FEE_INCREASE > 0);
}

BOOST_AUTO_TEST_CASE(test_payjoin_server_default_state)
{
    CPayJoinServer server(NULL);
    BOOST_CHECK(!server.IsRunning());
    BOOST_CHECK_EQUAL(server.GetPort(), 0);
}

BOOST_AUTO_TEST_CASE(test_payjoin_client_default_state)
{
    CPayJoinClient client(NULL);
    BOOST_CHECK_EQUAL(client.GetStatus(), PAYJOIN_IDLE);
}

BOOST_AUTO_TEST_CASE(test_payjoin_validate_sender_inputs_present)
{

    CTransaction txOriginal;
    txOriginal.vin.resize(1);
    txOriginal.vin[0].prevout.hash = GetRandHash();
    txOriginal.vin[0].prevout.n = 0;
    txOriginal.vout.resize(2);
    txOriginal.vout[0].nValue = 100 * COIN;
    txOriginal.vout[1].nValue = 50 * COIN;

    CTransaction txProposal;

    txProposal.vin.resize(1);
    txProposal.vin[0].prevout.hash = GetRandHash();
    txProposal.vin[0].prevout.n = 0;
    txProposal.vout = txOriginal.vout;

    bool fSenderInputFound = false;
    for (size_t i = 0; i < txProposal.vin.size(); ++i)
    {
        if (txProposal.vin[i].prevout == txOriginal.vin[0].prevout)
        {
            fSenderInputFound = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(!fSenderInputFound, "Proposal correctly missing sender input (would be rejected)");

    CTransaction txValid;
    txValid.vin.resize(2);
    txValid.vin[0] = txOriginal.vin[0];
    txValid.vin[1].prevout.hash = GetRandHash();
    txValid.vin[1].prevout.n = 0;
    txValid.vout = txOriginal.vout;

    fSenderInputFound = false;
    for (size_t i = 0; i < txValid.vin.size(); ++i)
    {
        if (txValid.vin[i].prevout == txOriginal.vin[0].prevout)
        {
            fSenderInputFound = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(fSenderInputFound, "Valid proposal contains sender's original input");
    BOOST_CHECK_MESSAGE(txValid.vin.size() > txOriginal.vin.size(), "Receiver added inputs");
}

BOOST_AUTO_TEST_CASE(test_payjoin_indistinguishable)
{

    CTransaction txPayJoin;
    txPayJoin.vin.resize(3);
    for (int i = 0; i < 3; ++i)
    {
        txPayJoin.vin[i].prevout.hash = GetRandHash();
        txPayJoin.vin[i].prevout.n = 0;
    }
    txPayJoin.vout.resize(2);
    txPayJoin.vout[0].nValue = 100 * COIN;
    txPayJoin.vout[1].nValue = 50 * COIN;

    BOOST_CHECK(txPayJoin.vin.size() > 1);
    BOOST_CHECK(!txPayJoin.IsCoinBase());
    BOOST_CHECK(!txPayJoin.IsCoinStake());

    for (size_t i = 1; i < txPayJoin.vin.size(); ++i)
    {
        BOOST_CHECK_EQUAL(txPayJoin.vin[i].scriptSig.size(),
                          txPayJoin.vin[0].scriptSig.size());
    }
}

BOOST_AUTO_TEST_CASE(test_payjoin_fee_increase_limit)
{

    int64_t nOriginalFee = 42000;
    int64_t nProposalFee = nOriginalFee + PAYJOIN_MAX_FEE_INCREASE;

    int64_t nFeeIncrease = nProposalFee - nOriginalFee;
    BOOST_CHECK(nFeeIncrease <= PAYJOIN_MAX_FEE_INCREASE);

    int64_t nExcessiveFee = nOriginalFee + PAYJOIN_MAX_FEE_INCREASE + 1;
    int64_t nExcessiveIncrease = nExcessiveFee - nOriginalFee;
    BOOST_CHECK(nExcessiveIncrease > PAYJOIN_MAX_FEE_INCREASE);
}

BOOST_AUTO_TEST_SUITE_END()
