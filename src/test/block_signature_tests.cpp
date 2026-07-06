#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "main.h"
#include "wallet.h"
#include "key.h"
#include "script.h"
#include "base58.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(block_signature_tests)

static CTransaction CreateCoinstakeWithoutCharity(CKey& key)
{
    CTransaction tx;
    tx.nTime = GetTime();

    tx.vin.resize(1);
    tx.vin[0].prevout.hash = uint256("1234567890123456789012345678901234567890123456789012345678901234");
    tx.vin[0].prevout.n = 0;

    CScript scriptEmpty;
    scriptEmpty.clear();
    tx.vout.push_back(CTxOut(0, scriptEmpty));

    CScript scriptPubKey;
    scriptPubKey << key.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(100 * COIN, scriptPubKey));

    return tx;
}

static CTransaction CreateCoinstakeWithCharity(CKey& stakerKey, CKey& charityKey)
{
    CTransaction tx;
    tx.nTime = GetTime();

    tx.vin.resize(1);
    tx.vin[0].prevout.hash = uint256("1234567890123456789012345678901234567890123456789012345678901234");
    tx.vin[0].prevout.n = 0;

    CScript scriptEmpty;
    scriptEmpty.clear();
    tx.vout.push_back(CTxOut(0, scriptEmpty));

    CScript scriptCharity;
    scriptCharity.SetDestination(charityKey.GetPubKey().GetID());
    tx.vout.push_back(CTxOut(10 * COIN, scriptCharity));

    CScript scriptPubKey;
    scriptPubKey << stakerKey.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(90 * COIN, scriptPubKey));

    return tx;
}

static CBlock CreatePoSBlock(const CTransaction& coinstake, CKey& key)
{
    CBlock block;
    block.nVersion = 7;
    block.nTime = GetTime();
    block.nBits = 0x1d00ffff;
    block.nNonce = 0;
    block.hashPrevBlock = 0;

    CTransaction coinbase;
    coinbase.nTime = block.nTime;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(2);
    coinbase.vout[0].scriptPubKey.clear();
    coinbase.vout[0].nValue = 0;
    coinbase.vout[1].scriptPubKey.clear();
    coinbase.vout[1].nValue = 0;
    block.vtx.push_back(coinbase);

    block.vtx.push_back(coinstake);

    block.hashMerkleRoot = block.BuildMerkleTree();

    uint256 hashBlock = block.GetHash();

    uint256 hashBlock2 = block.GetHash();
    if (hashBlock != hashBlock2)
    {
        BOOST_ERROR("Block hash changed between calls!");
    }

    if (!key.Sign(hashBlock, block.vchBlockSig))
    {
        BOOST_ERROR("Failed to sign block");
    }

    CKey verifyKey;
    vector<valtype> vSolutions;
    txnouttype whichType;
    bool foundStaker = false;
    for (unsigned int i = 1; i < coinstake.vout.size() && !foundStaker; i++)
    {
        txnouttype testType;
        vector<valtype> testSolutions;
        if (Solver(coinstake.vout[i].scriptPubKey, testType, testSolutions) && testType == TX_PUBKEY)
        {
            whichType = testType;
            vSolutions = testSolutions;
            foundStaker = true;
        }
    }

    if (!foundStaker && coinstake.vout.size() >= 2)
    {
        if (Solver(coinstake.vout[1].scriptPubKey, whichType, vSolutions) && whichType == TX_PUBKEY)
        {
            foundStaker = true;
        }
    }
    if (foundStaker && whichType == TX_PUBKEY && !vSolutions.empty())
    {
        verifyKey.SetPubKey(vSolutions[0]);
        bool immediateVerify = verifyKey.Verify(hashBlock, block.vchBlockSig);
        if (!immediateVerify)
        {
            BOOST_ERROR("Immediate signature verification failed - signing may be incorrect");
        }
    }
    else
    {
        BOOST_ERROR("Failed to find staker output (TX_PUBKEY) for immediate signature verification");
    }

    return block;
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_WithoutCharity)
{

    CKey stakerKey;
    stakerKey.MakeNewKey(false);

    CTransaction coinstake = CreateCoinstakeWithoutCharity(stakerKey);
    CBlock block = CreatePoSBlock(coinstake, stakerKey);

    BOOST_TEST_MESSAGE("Block vtx[1].vout.size() = " << block.vtx[1].vout.size());
    BOOST_TEST_MESSAGE("Block vchBlockSig.size() = " << block.vchBlockSig.size());
    BOOST_TEST_MESSAGE("Block hash = " << block.GetHash().ToString());

    BOOST_CHECK(block.vtx[1].vout.size() >= 2);
    BOOST_CHECK(block.vtx[1].vout[0].scriptPubKey.empty());
    BOOST_CHECK(!block.vtx[1].vout[1].scriptPubKey.empty());

    if (block.vchBlockSig.empty())
    {
        BOOST_ERROR("Block signature is empty - signing may have failed");
    }

    vector<valtype> vSolutions;
    txnouttype whichType;
    if (Solver(block.vtx[1].vout[1].scriptPubKey, whichType, vSolutions))
    {
        BOOST_TEST_MESSAGE("Staker output script type: " << whichType);
        BOOST_TEST_MESSAGE("Staker output solutions size: " << vSolutions.size());
        if (whichType == TX_PUBKEY && vSolutions.size() > 0)
        {
            CKey testKey;
            if (testKey.SetPubKey(vSolutions[0]))
            {
                BOOST_TEST_MESSAGE("Public key extracted successfully");
                BOOST_TEST_MESSAGE("PubKey matches stakerKey: " << (testKey.GetPubKey() == stakerKey.GetPubKey()));
            }
        }
    }

    bool sigResult = block.CheckBlockSignature();
    BOOST_TEST_MESSAGE("CheckBlockSignature() returned: " << sigResult);
    BOOST_CHECK(sigResult);
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_WithCharity)
{

    CKey stakerKey;
    stakerKey.MakeNewKey(false);

    CKey charityKey;
    charityKey.MakeNewKey(true);

    CTransaction coinstake = CreateCoinstakeWithCharity(stakerKey, charityKey);
    CBlock block = CreatePoSBlock(coinstake, stakerKey);

    BOOST_CHECK(block.CheckBlockSignature());

    BOOST_CHECK(block.vtx[1].vout.size() >= 3);
    BOOST_CHECK(block.vtx[1].vout[0].scriptPubKey.empty());

    vector<valtype> vSolutions;
    txnouttype whichType;
    BOOST_CHECK(Solver(block.vtx[1].vout[1].scriptPubKey, whichType, vSolutions));
    BOOST_CHECK(whichType == TX_PUBKEYHASH);

    BOOST_CHECK(Solver(block.vtx[1].vout[2].scriptPubKey, whichType, vSolutions));
    BOOST_CHECK(whichType == TX_PUBKEY);
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_WithMultipleCharity)
{

    CKey stakerKey;
    stakerKey.MakeNewKey(false);

    CKey charityKey1, charityKey2;
    charityKey1.MakeNewKey(true);
    charityKey2.MakeNewKey(true);

    CTransaction tx;
    tx.nTime = GetTime();

    tx.vin.resize(1);
    tx.vin[0].prevout.hash = uint256("1234567890123456789012345678901234567890123456789012345678901234");
    tx.vin[0].prevout.n = 0;

    CScript scriptEmpty;
    scriptEmpty.clear();
    tx.vout.push_back(CTxOut(0, scriptEmpty));

    CScript scriptCharity1;
    scriptCharity1.SetDestination(charityKey1.GetPubKey().GetID());
    tx.vout.push_back(CTxOut(5 * COIN, scriptCharity1));

    CScript scriptCharity2;
    scriptCharity2.SetDestination(charityKey2.GetPubKey().GetID());
    tx.vout.push_back(CTxOut(5 * COIN, scriptCharity2));

    CScript scriptPubKey;
    scriptPubKey << stakerKey.GetPubKey() << OP_CHECKSIG;
    tx.vout.push_back(CTxOut(90 * COIN, scriptPubKey));

    CBlock block = CreatePoSBlock(tx, stakerKey);

    BOOST_CHECK(block.CheckBlockSignature());

    BOOST_CHECK(block.vtx[1].vout.size() >= 4);
    vector<valtype> vSolutions;
    txnouttype whichType;
    BOOST_CHECK(Solver(block.vtx[1].vout[3].scriptPubKey, whichType, vSolutions));
    BOOST_CHECK(whichType == TX_PUBKEY);
}

BOOST_AUTO_TEST_CASE(CheckBlockSignature_InvalidSignature)
{

    CKey stakerKey;
    stakerKey.MakeNewKey(false);

    CTransaction coinstake = CreateCoinstakeWithoutCharity(stakerKey);
    CBlock block = CreatePoSBlock(coinstake, stakerKey);

    if (!block.vchBlockSig.empty())
    {
        block.vchBlockSig[0] ^= 0x01;

        BOOST_CHECK(!block.CheckBlockSignature());
    }
}

BOOST_AUTO_TEST_SUITE_END()
