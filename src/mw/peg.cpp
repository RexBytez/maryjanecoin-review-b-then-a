#ifdef ENABLE_MWEB

#include "mw/peg.h"
#include "mw/crypto/pedersen.h"
#include "mw/crypto/schnorr.h"
#include "mw/crypto/bulletproofs.h"
#include "util.h"
#include "base58.h"
#include "support/cleanse.h"

#include <openssl/sha.h>
#include <openssl/rand.h>

namespace mw {

static const unsigned char PEGIN_MARKER[] = { 0x4d, 0x57, 0x45, 0x42 };

CScript GetPegInMarkerScript(const Commitment& commitment)
{
    CScript script;
    script << OP_RETURN;

    std::vector<unsigned char> vData;
    vData.insert(vData.end(), PEGIN_MARKER, PEGIN_MARKER + sizeof(PEGIN_MARKER));
    vData.insert(vData.end(), commitment.data, commitment.data + COMMITMENT_SIZE);

    script << vData;
    return script;
}

bool ExtractPegInCommitment(const CScript& script, Commitment& commitmentOut)
{

    if (script.size() < 2 + sizeof(PEGIN_MARKER) + COMMITMENT_SIZE)
        return false;

    CScript::const_iterator it = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vData;

    if (!script.GetOp(it, opcode) || opcode != OP_RETURN)
        return false;

    if (!script.GetOp(it, opcode, vData))
        return false;

    if (vData.size() != sizeof(PEGIN_MARKER) + COMMITMENT_SIZE)
        return false;

    if (memcmp(vData.data(), PEGIN_MARKER, sizeof(PEGIN_MARKER)) != 0)
        return false;

    memcpy(commitmentOut.data, vData.data() + sizeof(PEGIN_MARKER), COMMITMENT_SIZE);
    return true;
}

CPegInResult CreatePegIn(CWallet* pWallet, int64_t nAmount)
{
    CPegInResult result;

    if (!pWallet)
    {
        result.strError = "wallet is null";
        return result;
    }

    if (nAmount <= 0)
    {
        result.strError = "amount must be positive";
        return result;
    }

    if (nAmount > MAX_MONEY)
    {
        result.strError = "amount exceeds maximum";
        return result;
    }

    crypto::PedersenContext& pedersen = crypto::PedersenContext::Get();

    BlindingFactor outputBlind = pedersen.GenerateBlindingFactor();
    if (outputBlind.IsNull())
    {
        result.strError = "failed to generate blinding factor";
        return result;
    }

    Commitment commitment = pedersen.Commit(nAmount, outputBlind);
    if (commitment.IsNull())
    {
        result.strError = "failed to create commitment";
        return result;
    }

    unsigned char senderPubKey[PUBKEY_SIZE];
    unsigned char receiverPubKey[PUBKEY_SIZE];

    {

        SecretKey senderSecret;
        RAND_bytes(senderSecret.data, 32);
        if (!crypto::SchnorrSigner::GetPublicKey(senderSecret, senderPubKey))
        {
            result.strError = "failed to generate sender key";
            return result;
        }

        SecretKey receiverSecret;
        RAND_bytes(receiverSecret.data, 32);
        if (!crypto::SchnorrSigner::GetPublicKey(receiverSecret, receiverPubKey))
        {
            result.strError = "failed to generate receiver key";
            return result;
        }
    }

    uint256 nonce;
    RAND_bytes((unsigned char*)&nonce, 32);
    RangeProof rangeProof = crypto::BulletproofProver::Prove(nAmount, outputBlind, nonce);
    if (rangeProof.IsNull())
    {
        result.strError = "failed to create range proof";
        return result;
    }

    result.mwOutput = CMWOutput(commitment, senderPubKey, receiverPubKey,
                                 rangeProof, OUTPUT_STANDARD);

    BlindingFactor excessBlind = outputBlind;
    Commitment excess = crypto::SchnorrSigner::GetExcessCommitment(excessBlind);

    result.kernel.nFeatures = KERNEL_PEGIN;
    result.kernel.nFee = 0;
    result.kernel.nPegAmount = nAmount;
    result.kernel.excess = excess;

    uint256 kernelMsg = result.kernel.GetSignatureMessage();
    result.kernel.signature = crypto::SchnorrSigner::SignExcess(excessBlind, kernelMsg);

    if (result.kernel.signature.IsNull())
    {
        result.strError = "failed to sign kernel";
        return result;
    }

    result.outputBlind = outputBlind;
    result.fSuccess = true;

    return result;
}

CPegOutResult CreatePegOut(CWallet* pWallet, int64_t nAmount,
                            const CTxDestination& dest)
{
    CPegOutResult result;

    if (!pWallet)
    {
        result.strError = "wallet is null";
        return result;
    }

    if (nAmount <= 0)
    {
        result.strError = "amount must be positive";
        return result;
    }

    result.destScript.SetDestination(dest);
    if (result.destScript.empty())
    {
        result.strError = "invalid destination address";
        return result;
    }

    crypto::PedersenContext& pedersen = crypto::PedersenContext::Get();

    BlindingFactor excessBlind = pedersen.GenerateBlindingFactor();
    Commitment excess = crypto::SchnorrSigner::GetExcessCommitment(excessBlind);

    result.kernel.nFeatures = KERNEL_PEGOUT;
    result.kernel.nFee = MIN_TX_FEE;
    result.kernel.nPegAmount = nAmount;
    result.kernel.excess = excess;
    result.kernel.pegoutScript = result.destScript;

    uint256 kernelMsg = result.kernel.GetSignatureMessage();
    result.kernel.signature = crypto::SchnorrSigner::SignExcess(excessBlind, kernelMsg);

    if (result.kernel.signature.IsNull())
    {
        result.strError = "failed to sign kernel";
        return result;
    }

    result.inputBlind = excessBlind;
    result.fSuccess = true;

    return result;
}

bool IsPegInTransaction(const CTransaction& tx, Commitment& commitmentOut,
                        int64_t& nAmountOut)
{
    nAmountOut = 0;

    for (size_t i = 0; i < tx.vout.size(); i++)
    {
        if (ExtractPegInCommitment(tx.vout[i].scriptPubKey, commitmentOut))
        {

            nAmountOut = tx.GetValueOut() - tx.vout[i].nValue;
            return true;
        }
    }

    return false;
}

bool ValidatePegOuts(const std::vector<CMWKernel>& vKernels,
                     const std::vector<CTxOut>& vTransparentOutputs)
{

    for (size_t i = 0; i < vKernels.size(); i++)
    {
        if (!vKernels[i].IsPegOut())
            continue;

        bool fFound = false;
        for (size_t j = 0; j < vTransparentOutputs.size(); j++)
        {
            if (vTransparentOutputs[j].scriptPubKey == vKernels[i].pegoutScript &&
                vTransparentOutputs[j].nValue == vKernels[i].nPegAmount)
            {
                fFound = true;
                break;
            }
        }

        if (!fFound)
        {
            printf("ValidatePegOuts() : PEGOUT kernel %zu has no matching transparent output\n", i);
            return false;
        }
    }

    return true;
}

}

#endif
