#ifndef MARYJANECOIN_MW_CONFIDENTIAL_H
#define MARYJANECOIN_MW_CONFIDENTIAL_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"
#include "mw/crypto/pedersen.h"
#include "mw/crypto/bulletproofs.h"
#include "mw/crypto/schnorr.h"

#include "mw/models/output.h"
#include "mw/models/input.h"
#include "mw/models/kernel.h"
#include "mw/models/tx_body.h"
#include "mw/models/block.h"

#include "mw/state/mw_state.h"
#include "mw/validation.h"

namespace mw {
    class CMWWallet;
    struct CMWOwnedOutput;
    struct CPegInResult;
    struct CPegOutResult;
}

static const int CT_TX_VERSION = 2;

static const int CT_MANDATORY_HEIGHT = 50000;

namespace ct {

using mw::BlindingFactor;
using mw::Commitment;
using mw::RangeProof;
using mw::Signature;
using mw::SecretKey;

using mw::CMWOutput;
using mw::CMWInput;
using mw::CMWKernel;
using mw::CMWTransaction;
using mw::CMWTransactionBody;
using mw::CMWBlock;

using mw::CMWState;

using mw::crypto::PedersenContext;
using mw::crypto::BulletproofProver;
using mw::crypto::BulletproofVerifier;
using mw::crypto::SchnorrSigner;
using mw::crypto::SchnorrVerifier;

struct CConfidentialTxOut
{
    Commitment commitment;
    RangeProof rangeProof;
    CScript scriptPubKey;
    unsigned char senderPubKey[mw::PUBKEY_SIZE];
    unsigned char receiverPubKey[mw::PUBKEY_SIZE];

    CConfidentialTxOut()
    {
        memset(senderPubKey, 0, mw::PUBKEY_SIZE);
        memset(receiverPubKey, 0, mw::PUBKEY_SIZE);
    }

    CMWOutput ToMWOutput() const
    {
        return CMWOutput(commitment, senderPubKey, receiverPubKey,
                         rangeProof, mw::OUTPUT_STANDARD);
    }

    static CConfidentialTxOut FromMWOutput(const CMWOutput& mwOut)
    {
        CConfidentialTxOut ct;
        ct.commitment = mwOut.commitment;
        ct.rangeProof = mwOut.rangeProof;
        memcpy(ct.senderPubKey, mwOut.senderPubKey, mw::PUBKEY_SIZE);
        memcpy(ct.receiverPubKey, mwOut.receiverPubKey, mw::PUBKEY_SIZE);
        return ct;
    }

    bool IsNull() const { return commitment.IsNull(); }

    IMPLEMENT_SERIALIZE(
        READWRITE(commitment);
        READWRITE(rangeProof);
        READWRITE(scriptPubKey);
        READWRITE(FLATDATA(senderPubKey));
        READWRITE(FLATDATA(receiverPubKey));
    )
};

}

#endif
#endif
