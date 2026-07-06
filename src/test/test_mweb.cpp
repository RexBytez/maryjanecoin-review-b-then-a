#define BOOST_TEST_MODULE MaryJaneCoin_MWEB_Tests
#include <boost/test/unit_test.hpp>

#include <vector>
#include <set>
#include <cstring>

#include "mw/mw_common.h"
#include "mw/crypto/pedersen.h"
#include "mw/crypto/schnorr.h"
#include "mw/crypto/bulletproofs.h"
#include "mw/models/kernel.h"
#include "mw/models/output.h"
#include "mw/models/input.h"
#include "mw/models/tx_body.h"
#include "mw/models/block.h"
#include "mw/state/mmr.h"
#include "mw/state/mw_state.h"
#include "key.h"
#include "uint256.h"
#include "hash.h"
#include "util.h"
#include "serialize.h"

static uint256 MakeRandomHash()
{
    return GetRandHash();
}

static mw::SecretKey SecretFromKey(const CKey& key)
{
    bool fCompressed = false;
    CSecret secret = key.GetSecret(fCompressed);
    mw::SecretKey sk;
    if (secret.size() >= 32)
        memcpy(sk.data, secret.data(), 32);
    return sk;
}

static mw::CMWOutput MakeOutput(const mw::Commitment& commit,
                                 const mw::RangeProof& proof = mw::RangeProof())
{
    mw::CMWOutput out;
    out.commitment = commit;
    out.rangeProof = proof;
    out.nFeatures  = mw::OUTPUT_STANDARD;

    memset(out.senderPubKey, 0x02, mw::PUBKEY_SIZE);
    memset(out.receiverPubKey, 0x03, mw::PUBKEY_SIZE);
    return out;
}

static mw::CMWInput MakeInput(const mw::Commitment& commit)
{
    mw::CMWInput inp;
    inp.commitment = commit;
    memset(inp.outputPubKey, 0x02, mw::PUBKEY_SIZE);
    return inp;
}

static mw::CMWKernel MakeKernel(uint8_t features, int64_t fee)
{
    mw::CMWKernel k;
    k.nFeatures = features;
    k.nFee = fee;
    return k;
}

static mw::Commitment FreshCommit(int64_t value)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    return ped.Commit(value, ped.GenerateBlindingFactor());
}

static mw::CMWBlock MakeMWBlock(int32_t nHeight,
                                const uint256& hashPrevMW,
                                int nNew,
                                const std::vector<mw::Commitment>& vSpend,
                                std::vector<mw::Commitment>& vCreatedOut)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::CMWBlock blk;
    blk.hashPrevMWBlock = hashPrevMW;
    blk.nHeight = nHeight;
    blk.offset = ped.GenerateBlindingFactor();

    for (int i = 0; i < nNew; i++)
    {
        mw::Commitment c = FreshCommit(1000 + nHeight * 10 + i);
        blk.body.vOutputs.push_back(MakeOutput(c));
        vCreatedOut.push_back(c);
    }
    for (size_t i = 0; i < vSpend.size(); i++)
        blk.body.vInputs.push_back(MakeInput(vSpend[i]));

    mw::CMWKernel k = MakeKernel(mw::KERNEL_PLAIN, 10);
    k.excess = ped.CommitBlind(ped.GenerateBlindingFactor());
    blk.body.vKernels.push_back(k);

    return blk;
}

BOOST_AUTO_TEST_SUITE(pedersen_tests)

BOOST_AUTO_TEST_CASE(test_pedersen_commit_nonzero)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();
    BOOST_REQUIRE(!blind.IsNull());

    mw::Commitment c = ped.Commit(42, blind);
    BOOST_CHECK(!c.IsNull());
}

BOOST_AUTO_TEST_CASE(test_pedersen_commit_deterministic)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();

    mw::Commitment c1 = ped.Commit(100, blind);
    mw::Commitment c2 = ped.Commit(100, blind);
    BOOST_CHECK(c1 == c2);
}

BOOST_AUTO_TEST_CASE(test_pedersen_commit_different_values)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();

    mw::Commitment c1 = ped.Commit(100, blind);
    mw::Commitment c2 = ped.Commit(200, blind);
    BOOST_CHECK(c1 != c2);
}

BOOST_AUTO_TEST_CASE(test_pedersen_sum_verification)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::BlindingFactor blindIn  = ped.GenerateBlindingFactor();
    mw::BlindingFactor blindOut = ped.GenerateBlindingFactor();

    int64_t amount = 5000;

    mw::Commitment cIn  = ped.Commit(amount, blindIn);
    mw::Commitment cOut = ped.Commit(amount, blindOut);

    mw::BlindingFactor excess = ped.BlindSum(
        std::vector<mw::BlindingFactor>(1, blindOut),
        std::vector<mw::BlindingFactor>(1, blindIn));

    mw::Commitment cExcess = ped.CommitBlind(excess);

    std::vector<mw::Commitment> positive;
    positive.push_back(cOut);
    std::vector<mw::Commitment> negative;
    negative.push_back(cIn);
    negative.push_back(cExcess);

    BOOST_CHECK(ped.VerifyCommitmentSum(positive, negative));
}

BOOST_AUTO_TEST_CASE(test_pedersen_sum_fails_unbalanced)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::BlindingFactor blindIn  = ped.GenerateBlindingFactor();
    mw::BlindingFactor blindOut = ped.GenerateBlindingFactor();

    mw::Commitment cIn  = ped.Commit(100, blindIn);
    mw::Commitment cOut = ped.Commit(200, blindOut);

    std::vector<mw::Commitment> positive(1, cOut);
    std::vector<mw::Commitment> negative(1, cIn);

    BOOST_CHECK(!ped.VerifyCommitmentSum(positive, negative));
}

BOOST_AUTO_TEST_CASE(test_pedersen_blind_add)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::BlindingFactor a = ped.GenerateBlindingFactor();
    mw::BlindingFactor b = ped.GenerateBlindingFactor();
    mw::BlindingFactor ab = ped.BlindAdd(a, b);

    BOOST_REQUIRE(!ab.IsNull());

    mw::Commitment cAB = ped.CommitBlind(ab);
    mw::Commitment cA  = ped.CommitBlind(a);
    mw::Commitment cB  = ped.CommitBlind(b);

    std::vector<mw::Commitment> positive(1, cAB);
    std::vector<mw::Commitment> negative;
    negative.push_back(cA);
    negative.push_back(cB);

    BOOST_CHECK(ped.VerifyCommitmentSum(positive, negative));
}

BOOST_AUTO_TEST_CASE(test_pedersen_blind_negate)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::BlindingFactor a   = ped.GenerateBlindingFactor();
    mw::BlindingFactor neg = ped.BlindNegate(a);

    BOOST_REQUIRE(!neg.IsNull());
    BOOST_CHECK(a != neg);

    mw::Commitment cA   = ped.CommitBlind(a);
    mw::Commitment cNeg = ped.CommitBlind(neg);

    std::vector<mw::Commitment> positive(1, cA);
    std::vector<mw::Commitment> negative(1, cNeg);

    BOOST_CHECK(ped.VerifyCommitmentSum(positive, negative));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(schnorr_tests)

BOOST_AUTO_TEST_CASE(test_schnorr_sign_verify)
{
    CKey key;
    key.MakeNewKey(true);
    mw::SecretKey sk = SecretFromKey(key);

    uint256 msg = MakeRandomHash();

    mw::Signature sig = mw::crypto::SchnorrSigner::Sign(sk, msg);
    BOOST_REQUIRE(!sig.IsNull());

    unsigned char pubkey33[33];
    BOOST_REQUIRE(mw::crypto::SchnorrSigner::GetPublicKey(sk, pubkey33));

    BOOST_CHECK(mw::crypto::SchnorrVerifier::Verify(pubkey33, msg, sig));
}

BOOST_AUTO_TEST_CASE(test_schnorr_wrong_key_fails)
{
    CKey key1;
    key1.MakeNewKey(true);
    CKey key2;
    key2.MakeNewKey(true);

    mw::SecretKey sk1 = SecretFromKey(key1);
    mw::SecretKey sk2 = SecretFromKey(key2);

    uint256 msg = MakeRandomHash();

    mw::Signature sig = mw::crypto::SchnorrSigner::Sign(sk1, msg);
    BOOST_REQUIRE(!sig.IsNull());

    unsigned char pubkey33_wrong[33];
    BOOST_REQUIRE(mw::crypto::SchnorrSigner::GetPublicKey(sk2, pubkey33_wrong));

    BOOST_CHECK(!mw::crypto::SchnorrVerifier::Verify(pubkey33_wrong, msg, sig));
}

BOOST_AUTO_TEST_CASE(test_schnorr_wrong_message_fails)
{
    CKey key;
    key.MakeNewKey(true);
    mw::SecretKey sk = SecretFromKey(key);

    uint256 msg1 = MakeRandomHash();
    uint256 msg2 = MakeRandomHash();

    mw::Signature sig = mw::crypto::SchnorrSigner::Sign(sk, msg1);
    BOOST_REQUIRE(!sig.IsNull());

    unsigned char pubkey33[33];
    BOOST_REQUIRE(mw::crypto::SchnorrSigner::GetPublicKey(sk, pubkey33));

    BOOST_CHECK(!mw::crypto::SchnorrVerifier::Verify(pubkey33, msg2, sig));
}

BOOST_AUTO_TEST_CASE(test_schnorr_excess_sign_verify)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    mw::BlindingFactor excess = ped.GenerateBlindingFactor();

    uint256 msg = MakeRandomHash();

    mw::Signature sig = mw::crypto::SchnorrSigner::SignExcess(excess, msg);
    BOOST_REQUIRE(!sig.IsNull());

    mw::Commitment excessCommit = mw::crypto::SchnorrSigner::GetExcessCommitment(excess);
    BOOST_REQUIRE(!excessCommit.IsNull());

    BOOST_CHECK(mw::crypto::SchnorrVerifier::VerifyExcess(excessCommit, msg, sig));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(bulletproof_tests)

BOOST_AUTO_TEST_CASE(test_bulletproof_prove_verify)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    int64_t value = 420000;
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();
    uint256 nonce = MakeRandomHash();

    mw::Commitment commit = ped.Commit(value, blind);
    BOOST_REQUIRE(!commit.IsNull());

    mw::RangeProof proof = mw::crypto::BulletproofProver::Prove(value, blind, nonce);
    BOOST_REQUIRE(!proof.IsNull());

    BOOST_CHECK(mw::crypto::BulletproofVerifier::Verify(commit, proof));
}

BOOST_AUTO_TEST_CASE(test_bulletproof_wrong_commitment_fails)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    int64_t value = 100;
    mw::BlindingFactor blind1 = ped.GenerateBlindingFactor();
    mw::BlindingFactor blind2 = ped.GenerateBlindingFactor();
    uint256 nonce = MakeRandomHash();

    mw::Commitment commit1 = ped.Commit(value, blind1);
    mw::Commitment commit2 = ped.Commit(value, blind2);

    mw::RangeProof proof = mw::crypto::BulletproofProver::Prove(value, blind1, nonce);
    BOOST_REQUIRE(!proof.IsNull());

    BOOST_CHECK(!mw::crypto::BulletproofVerifier::Verify(commit2, proof));
}

BOOST_AUTO_TEST_CASE(test_bulletproof_proof_size)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    int64_t value = 1000000;
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();
    uint256 nonce = MakeRandomHash();

    mw::RangeProof proof = mw::crypto::BulletproofProver::Prove(value, blind, nonce);
    BOOST_REQUIRE(!proof.IsNull());

    size_t sz = proof.GetSize();
    BOOST_CHECK_GE(sz, (size_t)500);
    BOOST_CHECK_LE(sz, mw::MAX_RANGEPROOF_SIZE);

    BOOST_CHECK_GE(sz, (size_t)650);
    BOOST_CHECK_LE(sz, (size_t)750);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(mw_transaction_model_tests)

BOOST_AUTO_TEST_CASE(test_kernel_signature_message)
{
    mw::CMWKernel k;
    k.nFeatures = mw::KERNEL_PLAIN;
    k.nFee = 1000;

    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();
    k.excess = ped.CommitBlind(blind);

    uint256 msg1 = k.GetSignatureMessage();
    uint256 msg2 = k.GetSignatureMessage();
    BOOST_CHECK(msg1 == msg2);
    BOOST_CHECK(msg1 != uint256(0));
}

BOOST_AUTO_TEST_CASE(test_kernel_features)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();
    mw::Commitment excess = ped.CommitBlind(blind);

    mw::CMWKernel kPlain;
    kPlain.nFeatures = mw::KERNEL_PLAIN;
    kPlain.nFee = 100;
    kPlain.excess = excess;

    mw::CMWKernel kPegIn;
    kPegIn.nFeatures = mw::KERNEL_PEGIN;
    kPegIn.nFee = 100;
    kPegIn.nPegAmount = 5000;
    kPegIn.excess = excess;

    mw::CMWKernel kPegOut;
    kPegOut.nFeatures = mw::KERNEL_PEGOUT;
    kPegOut.nFee = 100;
    kPegOut.nPegAmount = 3000;
    kPegOut.excess = excess;

    uint256 msgPlain  = kPlain.GetSignatureMessage();
    uint256 msgPegIn  = kPegIn.GetSignatureMessage();
    uint256 msgPegOut = kPegOut.GetSignatureMessage();

    BOOST_CHECK(msgPlain != msgPegIn);
    BOOST_CHECK(msgPlain != msgPegOut);
    BOOST_CHECK(msgPegIn != msgPegOut);

    BOOST_CHECK(kPlain.IsPlain());
    BOOST_CHECK(!kPlain.IsPegIn());
    BOOST_CHECK(kPegIn.IsPegIn());
    BOOST_CHECK(!kPegIn.IsPegOut());
    BOOST_CHECK(kPegOut.IsPegOut());
    BOOST_CHECK(!kPegOut.IsPlain());
}

BOOST_AUTO_TEST_CASE(test_output_id_deterministic)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    mw::BlindingFactor blind = ped.GenerateBlindingFactor();
    mw::Commitment commit = ped.Commit(500, blind);

    mw::CMWOutput out = MakeOutput(commit);

    uint256 id1 = out.GetOutputID();
    uint256 id2 = out.GetOutputID();
    BOOST_CHECK(id1 == id2);
    BOOST_CHECK(id1 != uint256(0));

    mw::BlindingFactor blind2 = ped.GenerateBlindingFactor();
    mw::Commitment commit2 = ped.Commit(500, blind2);
    mw::CMWOutput out2 = MakeOutput(commit2);

    BOOST_CHECK(out.GetOutputID() != out2.GetOutputID());
}

BOOST_AUTO_TEST_CASE(test_tx_body_merge)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::BlindingFactor bA = ped.GenerateBlindingFactor();
    mw::Commitment cA = ped.Commit(100, bA);

    mw::CMWTransactionBody bodyA;
    bodyA.vInputs.push_back(MakeInput(cA));
    bodyA.vOutputs.push_back(MakeOutput(cA));
    bodyA.vKernels.push_back(MakeKernel(mw::KERNEL_PLAIN, 10));

    mw::BlindingFactor bB1 = ped.GenerateBlindingFactor();
    mw::BlindingFactor bB2 = ped.GenerateBlindingFactor();
    mw::Commitment cB1 = ped.Commit(200, bB1);
    mw::Commitment cB2 = ped.Commit(300, bB2);

    mw::CMWTransactionBody bodyB;
    bodyB.vInputs.push_back(MakeInput(cB1));
    bodyB.vInputs.push_back(MakeInput(cB2));
    bodyB.vOutputs.push_back(MakeOutput(cB2));
    bodyB.vKernels.push_back(MakeKernel(mw::KERNEL_PLAIN, 20));

    bodyA.Merge(bodyB);

    BOOST_CHECK_EQUAL(bodyA.vInputs.size(), (size_t)3);
    BOOST_CHECK_EQUAL(bodyA.vOutputs.size(), (size_t)2);
    BOOST_CHECK_EQUAL(bodyA.vKernels.size(), (size_t)2);
}

BOOST_AUTO_TEST_CASE(test_tx_body_cut_through)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::BlindingFactor b1 = ped.GenerateBlindingFactor();
    mw::BlindingFactor b2 = ped.GenerateBlindingFactor();
    mw::BlindingFactor b3 = ped.GenerateBlindingFactor();

    mw::Commitment c1 = ped.Commit(100, b1);
    mw::Commitment c2 = ped.Commit(200, b2);
    mw::Commitment c3 = ped.Commit(300, b3);

    mw::CMWTransactionBody body;
    body.vInputs.push_back(MakeInput(c1));
    body.vInputs.push_back(MakeInput(c2));
    body.vOutputs.push_back(MakeOutput(c2));
    body.vOutputs.push_back(MakeOutput(c3));
    body.vKernels.push_back(MakeKernel(mw::KERNEL_PLAIN, 5));

    std::vector<mw::Commitment> cut = body.ApplyCutThrough();

    BOOST_CHECK_EQUAL(cut.size(), (size_t)1);
    BOOST_CHECK(cut[0] == c2);

    BOOST_CHECK_EQUAL(body.vInputs.size(), (size_t)1);
    BOOST_CHECK_EQUAL(body.vOutputs.size(), (size_t)1);
    BOOST_CHECK(body.vInputs[0].commitment == c1);
    BOOST_CHECK(body.vOutputs[0].commitment == c3);
    BOOST_CHECK_EQUAL(body.vKernels.size(), (size_t)1);
}

BOOST_AUTO_TEST_CASE(test_tx_body_fees)
{
    mw::CMWTransactionBody body;
    body.vKernels.push_back(MakeKernel(mw::KERNEL_PLAIN, 100));
    body.vKernels.push_back(MakeKernel(mw::KERNEL_PLAIN, 250));
    body.vKernels.push_back(MakeKernel(mw::KERNEL_PEGIN, 50));

    BOOST_CHECK_EQUAL(body.GetTotalFee(), (int64_t)400);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(mmr_tests)

BOOST_AUTO_TEST_CASE(test_mmr_empty_root)
{
    mw::CMMR mmr;
    BOOST_CHECK(mmr.IsEmpty());
    BOOST_CHECK_EQUAL(mmr.GetSize(), (uint64_t)0);
    BOOST_CHECK(mmr.GetRoot() == uint256(0));
}

BOOST_AUTO_TEST_CASE(test_mmr_single_leaf)
{
    mw::CMMR mmr;
    uint256 leaf = MakeRandomHash();

    uint64_t idx = mmr.Append(leaf);
    BOOST_CHECK_EQUAL(idx, (uint64_t)0);
    BOOST_CHECK_EQUAL(mmr.GetSize(), (uint64_t)1);

    uint256 root = mmr.GetRoot();
    BOOST_CHECK(root != uint256(0));

    mw::CMMR mmr2;
    mmr2.Append(leaf);
    BOOST_CHECK(mmr2.GetRoot() == root);
}

BOOST_AUTO_TEST_CASE(test_mmr_append_changes_root)
{
    mw::CMMR mmr;

    std::set<uint256> roots;
    roots.insert(mmr.GetRoot());

    for (int i = 0; i < 10; ++i)
    {
        mmr.Append(MakeRandomHash());
        uint256 r = mmr.GetRoot();

        BOOST_CHECK(roots.find(r) == roots.end());
        roots.insert(r);
    }

    BOOST_CHECK_EQUAL(roots.size(), (size_t)11);
}

BOOST_AUTO_TEST_CASE(test_mmr_proof_verify)
{
    mw::CMMR mmr;

    std::vector<uint256> leaves;
    for (int i = 0; i < 8; ++i)
    {
        uint256 h = MakeRandomHash();
        leaves.push_back(h);
        mmr.Append(h);
    }

    uint256 root = mmr.GetRoot();

    for (uint64_t i = 0; i < leaves.size(); ++i)
    {
        mw::CMMRProof proof = mmr.GetProof(i);
        BOOST_CHECK(!proof.IsNull());
        BOOST_CHECK(mw::CMMR::VerifyProof(root, leaves[i], proof));
    }
}

BOOST_AUTO_TEST_CASE(test_mmr_wrong_proof_fails)
{
    mw::CMMR mmr;

    uint256 realLeaf = MakeRandomHash();
    uint256 fakeLeaf = MakeRandomHash();

    mmr.Append(realLeaf);
    mmr.Append(MakeRandomHash());
    mmr.Append(MakeRandomHash());
    mmr.Append(MakeRandomHash());

    uint256 root = mmr.GetRoot();

    mw::CMMRProof proof = mmr.GetProof(0);

    BOOST_CHECK(mw::CMMR::VerifyProof(root, realLeaf, proof));

    BOOST_CHECK(!mw::CMMR::VerifyProof(root, fakeLeaf, proof));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(mw_block_tests)

BOOST_AUTO_TEST_CASE(test_mw_block_hash_deterministic)
{
    mw::CMWBlock block;
    block.hashPrevMWBlock = MakeRandomHash();
    block.nHeight = 1000;

    block.body.vKernels.push_back(MakeKernel(mw::KERNEL_PLAIN, 50));

    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();
    block.offset = ped.GenerateBlindingFactor();

    uint256 h1 = block.GetHash();
    uint256 h2 = block.GetHash();
    BOOST_CHECK(h1 == h2);
    BOOST_CHECK(h1 != uint256(0));
}

BOOST_AUTO_TEST_CASE(test_mw_block_serialization)
{
    mw::crypto::PedersenContext& ped = mw::crypto::PedersenContext::Get();

    mw::CMWBlock block;
    block.hashPrevMWBlock = MakeRandomHash();
    block.nHeight = 42;
    block.offset = ped.GenerateBlindingFactor();

    mw::CMWKernel kernel = MakeKernel(mw::KERNEL_PLAIN, 100);
    kernel.excess = ped.CommitBlind(ped.GenerateBlindingFactor());
    block.body.vKernels.push_back(kernel);

    mw::BlindingFactor blindOut = ped.GenerateBlindingFactor();
    mw::Commitment commitOut = ped.Commit(500, blindOut);
    block.body.vOutputs.push_back(MakeOutput(commitOut));

    mw::BlindingFactor blindIn = ped.GenerateBlindingFactor();
    mw::Commitment commitIn = ped.Commit(300, blindIn);
    block.body.vInputs.push_back(MakeInput(commitIn));

    CDataStream ss(SER_DISK, 0);
    ss << block;

    mw::CMWBlock block2;
    ss >> block2;

    BOOST_CHECK(block2.hashPrevMWBlock == block.hashPrevMWBlock);
    BOOST_CHECK_EQUAL(block2.nHeight, block.nHeight);
    BOOST_CHECK_EQUAL(block2.body.vKernels.size(), block.body.vKernels.size());
    BOOST_CHECK_EQUAL(block2.body.vOutputs.size(), block.body.vOutputs.size());
    BOOST_CHECK_EQUAL(block2.body.vInputs.size(), block.body.vInputs.size());

    BOOST_CHECK(block2.GetHash() == block.GetHash());

    BOOST_CHECK_EQUAL(block2.body.vKernels[0].nFee, (int64_t)100);
    BOOST_CHECK(block2.body.vKernels[0].excess == kernel.excess);

    BOOST_CHECK(block2.body.vOutputs[0].commitment == commitOut);

    BOOST_CHECK(block2.body.vInputs[0].commitment == commitIn);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(mweb_reorg_tests)

BOOST_AUTO_TEST_CASE(test_mmr_node_count_formula)
{
    BOOST_CHECK_EQUAL(mw::CMMR::NodeCountForLeaves(0), (uint64_t)0);
    BOOST_CHECK_EQUAL(mw::CMMR::NodeCountForLeaves(1), (uint64_t)1);
    BOOST_CHECK_EQUAL(mw::CMMR::NodeCountForLeaves(2), (uint64_t)3);
    BOOST_CHECK_EQUAL(mw::CMMR::NodeCountForLeaves(3), (uint64_t)4);
    BOOST_CHECK_EQUAL(mw::CMMR::NodeCountForLeaves(4), (uint64_t)7);
    BOOST_CHECK_EQUAL(mw::CMMR::NodeCountForLeaves(7), (uint64_t)11);
    BOOST_CHECK_EQUAL(mw::CMMR::NodeCountForLeaves(8), (uint64_t)15);

    mw::CMMR mmr;
    for (uint64_t n = 1; n <= 64; n++)
    {
        mmr.Append(MakeRandomHash());
        BOOST_REQUIRE_EQUAL(mmr.GetSize(), n);
    }
}

BOOST_AUTO_TEST_CASE(test_mmr_rewind_byte_exact)
{
    const int N = 50;
    std::vector<uint256> leaves;
    std::vector<uint256> rootAt;

    mw::CMMR growing;
    rootAt.push_back(growing.GetRoot());
    for (int i = 0; i < N; i++)
    {
        uint256 h = MakeRandomHash();
        leaves.push_back(h);
        growing.Append(h);
        rootAt.push_back(growing.GetRoot());
    }

    for (int target = N; target >= 0; target--)
    {
        mw::CMMR m;
        for (int i = 0; i < N; i++)
            m.Append(leaves[i]);

        BOOST_REQUIRE(m.Rewind((uint64_t)target));
        BOOST_REQUIRE_EQUAL(m.GetSize(), (uint64_t)target);
        BOOST_CHECK(m.GetRoot() == rootAt[target]);

        if (target > 0)
        {
            mw::CMMRProof p = m.GetProof((uint64_t)target - 1);
            BOOST_CHECK(mw::CMMR::VerifyProof(m.GetRoot(), leaves[target - 1], p));
        }
    }

    mw::CMMR small;
    small.Append(MakeRandomHash());
    BOOST_CHECK(!small.Rewind(5));
}

BOOST_AUTO_TEST_CASE(test_mwstate_apply_disconnect_inverse)
{
    mw::CMWState st;
    uint256 hEmpty = st.ComputeStateHash();

    std::vector<mw::Commitment> created;
    std::vector<mw::Commitment> noSpend;
    mw::CMWBlock blk = MakeMWBlock(50000, uint256(0), 3, noSpend, created);

    uint256 blockHash = MakeRandomHash();
    BOOST_REQUIRE(st.ApplyBlock(blk, 50000, blockHash, 3000));
    BOOST_CHECK_EQUAL(st.GetOutputCount(), (size_t)3);
    BOOST_CHECK(st.HasUndo(blockHash));
    BOOST_CHECK(st.ComputeStateHash() != hEmpty);

    BOOST_CHECK(!st.ApplyBlock(blk, 50000, blockHash, 3000));

    BOOST_REQUIRE(st.DisconnectBlock(blockHash, false));
    BOOST_CHECK_EQUAL(st.GetOutputCount(), (size_t)0);
    BOOST_CHECK(!st.HasUndo(blockHash));
    BOOST_CHECK(st.ComputeStateHash() == hEmpty);
}

BOOST_AUTO_TEST_CASE(test_mwstate_disconnect_missing_undo_semantics)
{
    mw::CMWState st;
    uint256 ghost = MakeRandomHash();

    BOOST_CHECK(st.DisconnectBlock(ghost, true));

    BOOST_CHECK(!st.DisconnectBlock(ghost, false));
}

BOOST_AUTO_TEST_CASE(test_mwstate_spend_reorg_respend)
{
    mw::CMWState st;

    std::vector<mw::Commitment> created1;
    std::vector<mw::Commitment> noSpend;
    mw::CMWBlock blk1 = MakeMWBlock(50000, uint256(0), 3, noSpend, created1);
    uint256 h1 = MakeRandomHash();
    BOOST_REQUIRE(st.ApplyBlock(blk1, 50000, h1, 3000));

    mw::Commitment X = created1[0];
    BOOST_REQUIRE(st.HasOutput(X));
    uint256 fpAfterCreate = st.ComputeStateHash();

    std::vector<mw::Commitment> created2;
    std::vector<mw::Commitment> spendX(1, X);
    mw::CMWBlock blk2 = MakeMWBlock(50001, blk1.GetHash(), 1, spendX, created2);
    uint256 h2 = MakeRandomHash();
    BOOST_REQUIRE(st.ApplyBlock(blk2, 50001, h2, 1000));
    BOOST_CHECK(!st.HasOutput(X));

    BOOST_REQUIRE(st.DisconnectBlock(h2, false));
    BOOST_CHECK(st.HasOutput(X));
    BOOST_CHECK(st.ComputeStateHash() == fpAfterCreate);

    std::vector<mw::Commitment> created2b;
    mw::CMWBlock blk2b = MakeMWBlock(50001, blk1.GetHash(), 1, spendX, created2b);
    uint256 h2b = MakeRandomHash();
    BOOST_REQUIRE(st.ApplyBlock(blk2b, 50001, h2b, 1000));
    BOOST_CHECK(!st.HasOutput(X));
}

BOOST_AUTO_TEST_CASE(test_mwstate_100_clean_reorg_cycles)
{
    mw::CMWState st;

    std::vector<mw::Commitment> live;
    uint256 prevMW(0);
    int32_t height = 50000;
    for (int b = 0; b < 20; b++)
    {
        std::vector<mw::Commitment> spend;
        if (!live.empty() && (b % 2 == 0))
        {
            spend.push_back(live.back());
            live.pop_back();
        }
        std::vector<mw::Commitment> created;
        mw::CMWBlock blk = MakeMWBlock(height, prevMW, 2, spend, created);
        uint256 bh = MakeRandomHash();
        BOOST_REQUIRE(st.ApplyBlock(blk, height, bh, 2000));
        for (size_t i = 0; i < created.size(); i++)
            live.push_back(created[i]);
        prevMW = blk.GetHash();
        height++;
    }

    const uint256 fork = st.ComputeStateHash();
    const uint256 forkPrevMW = prevMW;
    const int32_t forkHeight = height;

    int nCleanCycles = 0;
    for (int cycle = 0; cycle < 100; cycle++)
    {
        int depth = (cycle % 4) + 1;

        std::vector<uint256> branchHashes;
        std::vector<mw::Commitment> branchLive = live;
        uint256 bprev = forkPrevMW;
        int32_t bh = forkHeight;
        mw::Commitment lastCreated;
        bool haveLastCreated = false;

        for (int d = 0; d < depth; d++)
        {
            std::vector<mw::Commitment> spend;
            if (d == 0 && !branchLive.empty())
            {
                spend.push_back(branchLive.back());
                branchLive.pop_back();
            }
            else if (d > 0 && haveLastCreated)
            {
                spend.push_back(lastCreated);
            }

            std::vector<mw::Commitment> created;
            mw::CMWBlock blk = MakeMWBlock(bh, bprev, 2, spend, created);
            uint256 chash = MakeRandomHash();
            BOOST_REQUIRE(st.ApplyBlock(blk, bh, chash, 2000));
            branchHashes.push_back(chash);

            lastCreated = created.front();
            haveLastCreated = true;
            bprev = blk.GetHash();
            bh++;
        }

        for (int d = depth - 1; d >= 0; d--)
        {
            BOOST_REQUIRE(st.DisconnectBlock(branchHashes[d], false));
        }

        BOOST_REQUIRE_MESSAGE(st.ComputeStateHash() == fork,
            "reorg cycle " << cycle << " (depth " << depth << ") diverged from fork point");
        nCleanCycles++;
    }

    BOOST_CHECK_EQUAL(nCleanCycles, 100);

    BOOST_CHECK(st.GetLatestMWBlockHash() == forkPrevMW);
    BOOST_CHECK_EQUAL(st.GetHeight(), forkHeight - 1);
}

BOOST_AUTO_TEST_SUITE_END()
