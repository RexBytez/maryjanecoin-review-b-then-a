#ifdef ENABLE_MWEB

#include "mw/validation.h"
#include "mw/crypto/pedersen.h"
#include "mw/crypto/schnorr.h"
#include "mw/crypto/bulletproofs.h"
#include "util.h"

namespace mw {

std::string CMWValidationResult::ToString() const
{
    if (IsValid())
        return "MW validation passed";

    std::string str = "MW validation failed: ";
    switch (error)
    {
        case MW_ERR_INVALID_RANGEPROOF:  str += "invalid range proof"; break;
        case MW_ERR_INVALID_SIGNATURE:   str += "invalid kernel signature"; break;
        case MW_ERR_COMMITMENT_SUM:      str += "commitment sum mismatch"; break;
        case MW_ERR_DOUBLE_SPEND:        str += "double spend"; break;
        case MW_ERR_OUTPUT_NOT_FOUND:    str += "output not found"; break;
        case MW_ERR_INVALID_CUTTHROUGH:  str += "invalid cut-through"; break;
        case MW_ERR_PEG_MISMATCH:        str += "peg amount mismatch"; break;
        case MW_ERR_INVALID_FEE:         str += "invalid fee"; break;
        case MW_ERR_INVALID_KERNEL:      str += "invalid kernel"; break;
        case MW_ERR_HEIGHT_LOCKED:       str += "height-locked"; break;
        case MW_ERR_PREV_BLOCK_MISMATCH: str += "previous block mismatch"; break;
        case MW_ERR_INFLATION:           str += "inflation detected"; break;
        case MW_ERR_DUPLICATE_OUTPUT:    str += "duplicate output"; break;
        case MW_ERR_EMPTY_BLOCK:         str += "empty block"; break;
        default: str += "unknown error"; break;
    }

    if (!strMessage.empty())
        str += " (" + strMessage + ")";

    if (nKernelIndex >= 0)
        str += strprintf(" kernel[%d]", nKernelIndex);
    if (nOutputIndex >= 0)
        str += strprintf(" output[%d]", nOutputIndex);
    if (nInputIndex >= 0)
        str += strprintf(" input[%d]", nInputIndex);

    return str;
}

bool VerifyKernelSignatures(const CMWTransactionBody& body)
{
    for (size_t i = 0; i < body.vKernels.size(); i++)
    {
        const CMWKernel& kernel = body.vKernels[i];

        uint256 msg = kernel.GetSignatureMessage();

        if (!crypto::SchnorrVerifier::VerifyExcess(kernel.excess, msg, kernel.signature))
        {
            printf("VerifyKernelSignatures() : kernel %zu signature invalid\n", i);
            return false;
        }
    }

    return true;
}

bool VerifyRangeProofs(const CMWTransactionBody& body)
{
    if (body.vOutputs.empty())
        return true;

    std::vector<std::pair<Commitment, RangeProof>> vProofs;
    vProofs.reserve(body.vOutputs.size());

    for (size_t i = 0; i < body.vOutputs.size(); i++)
    {
        const CMWOutput& output = body.vOutputs[i];

        if (output.rangeProof.IsNull())
        {
            printf("VerifyRangeProofs() : output %zu has no range proof\n", i);
            return false;
        }

        vProofs.push_back(std::make_pair(output.commitment, output.rangeProof));
    }

    return crypto::BulletproofVerifier::BatchVerify(vProofs);
}

bool VerifyCommitmentSum(
    const CMWTransactionBody& body,
    const BlindingFactor& offset)
{
    crypto::PedersenContext& pedersen = crypto::PedersenContext::Get();

    std::vector<Commitment> vPositive;
    for (size_t i = 0; i < body.vOutputs.size(); i++)
    {
        vPositive.push_back(body.vOutputs[i].commitment);
    }

    std::vector<Commitment> vNegative;
    for (size_t i = 0; i < body.vInputs.size(); i++)
    {
        vNegative.push_back(body.vInputs[i].commitment);
    }

    for (size_t i = 0; i < body.vKernels.size(); i++)
    {
        vNegative.push_back(body.vKernels[i].excess);
    }

    if (!offset.IsNull())
    {
        Commitment offsetCommit = pedersen.CommitBlind(offset);
        vNegative.push_back(offsetCommit);
    }

    int64_t nTotalFee = body.GetTotalFee();
    if (nTotalFee > 0)
    {
        BlindingFactor zeroBlind;
        Commitment feeCommit = pedersen.Commit(nTotalFee, zeroBlind);
        vNegative.push_back(feeCommit);
    }

    int64_t nPegIn = body.GetTotalPegIn();
    if (nPegIn > 0)
    {
        BlindingFactor zeroBlind;
        Commitment pegInCommit = pedersen.Commit(nPegIn, zeroBlind);
        vNegative.push_back(pegInCommit);
    }

    int64_t nPegOut = body.GetTotalPegOut();
    if (nPegOut > 0)
    {
        BlindingFactor zeroBlind;
        Commitment pegOutCommit = pedersen.Commit(nPegOut, zeroBlind);
        vPositive.push_back(pegOutCommit);
    }

    return pedersen.VerifyCommitmentSum(vPositive, vNegative);
}

CMWValidationResult ValidateMWBlock(
    const CMWBlock& block,
    const CMWState& state,
    int nBlockHeight,
    int64_t nTransparentPegIn,
    int64_t nTransparentPegOut)
{
    CMWValidationResult result;

    if (block.hashPrevMWBlock != state.GetLatestMWBlockHash())
    {
        result.error = MW_ERR_PREV_BLOCK_MISMATCH;
        result.strMessage = "previous MW block hash mismatch";
        return result;
    }

    if (!VerifyRangeProofs(block.body))
    {
        result.error = MW_ERR_INVALID_RANGEPROOF;
        result.strMessage = "one or more range proofs failed";
        return result;
    }

    if (!VerifyKernelSignatures(block.body))
    {
        result.error = MW_ERR_INVALID_SIGNATURE;
        result.strMessage = "one or more kernel signatures failed";
        return result;
    }

    if (!VerifyCommitmentSum(block.body, block.offset))
    {
        result.error = MW_ERR_COMMITMENT_SUM;
        result.strMessage = "commitment sum does not balance";
        return result;
    }

    for (size_t i = 0; i < block.body.vInputs.size(); i++)
    {
        const CMWInput& input = block.body.vInputs[i];
        if (!state.HasOutput(input.commitment))
        {
            result.error = MW_ERR_OUTPUT_NOT_FOUND;
            result.nInputIndex = (int)i;
            result.strMessage = "input references non-existent output";
            return result;
        }
    }

    {
        std::set<Commitment> inputCommitments;
        for (size_t i = 0; i < block.body.vInputs.size(); i++)
        {
            if (!inputCommitments.insert(block.body.vInputs[i].commitment).second)
            {
                result.error = MW_ERR_DOUBLE_SPEND;
                result.nInputIndex = (int)i;
                result.strMessage = "duplicate input commitment in block";
                return result;
            }
        }
    }

    {
        std::set<Commitment> outputCommitments;
        for (size_t i = 0; i < block.body.vOutputs.size(); i++)
        {
            if (!outputCommitments.insert(block.body.vOutputs[i].commitment).second)
            {
                result.error = MW_ERR_DUPLICATE_OUTPUT;
                result.nOutputIndex = (int)i;
                result.strMessage = "duplicate output commitment in block";
                return result;
            }
        }
    }

    for (size_t i = 0; i < block.body.vKernels.size(); i++)
    {
        const CMWKernel& kernel = block.body.vKernels[i];

        if (kernel.IsHeightLocked() && kernel.nLockHeight > nBlockHeight)
        {
            result.error = MW_ERR_HEIGHT_LOCKED;
            result.nKernelIndex = (int)i;
            result.strMessage = strprintf("kernel locked until height %d, current height %d",
                                          kernel.nLockHeight, nBlockHeight);
            return result;
        }

        if (kernel.nFee < 0)
        {
            result.error = MW_ERR_INVALID_FEE;
            result.nKernelIndex = (int)i;
            result.strMessage = "negative fee";
            return result;
        }

        if ((kernel.IsPegIn() || kernel.IsPegOut()) && kernel.nPegAmount <= 0)
        {
            result.error = MW_ERR_INVALID_KERNEL;
            result.nKernelIndex = (int)i;
            result.strMessage = "peg kernel with non-positive amount";
            return result;
        }
    }

    int64_t nMWPegIn = block.body.GetTotalPegIn();
    int64_t nMWPegOut = block.body.GetTotalPegOut();

    if (nMWPegIn != nTransparentPegIn)
    {
        result.error = MW_ERR_PEG_MISMATCH;
        result.strMessage = strprintf("peg-in mismatch: MW=%lld transparent=%lld",
                                       nMWPegIn, nTransparentPegIn);
        return result;
    }

    if (nMWPegOut != nTransparentPegOut)
    {
        result.error = MW_ERR_PEG_MISMATCH;
        result.strMessage = strprintf("peg-out mismatch: MW=%lld transparent=%lld",
                                       nMWPegOut, nTransparentPegOut);
        return result;
    }

    uint256 txHash = block.body.GetHash();
    for (size_t i = 0; i < block.body.vInputs.size(); i++)
    {
        if (!block.body.vInputs[i].VerifySignature(txHash))
        {
            result.error = MW_ERR_INVALID_SIGNATURE;
            result.nInputIndex = (int)i;
            result.strMessage = "input signature verification failed";
            return result;
        }
    }

    return result;
}

CMWValidationResult ValidateMWTransaction(
    const CMWTransaction& tx,
    const CMWState& state,
    int nBlockHeight)
{
    CMWValidationResult result;

    if (tx.IsNull())
    {
        result.error = MW_ERR_EMPTY_BLOCK;
        result.strMessage = "empty transaction";
        return result;
    }

    if (!VerifyRangeProofs(tx.body))
    {
        result.error = MW_ERR_INVALID_RANGEPROOF;
        return result;
    }

    if (!VerifyKernelSignatures(tx.body))
    {
        result.error = MW_ERR_INVALID_SIGNATURE;
        return result;
    }

    if (!VerifyCommitmentSum(tx.body, tx.offset))
    {
        result.error = MW_ERR_COMMITMENT_SUM;
        return result;
    }

    for (size_t i = 0; i < tx.body.vInputs.size(); i++)
    {
        if (!state.HasOutput(tx.body.vInputs[i].commitment))
        {
            result.error = MW_ERR_OUTPUT_NOT_FOUND;
            result.nInputIndex = (int)i;
            return result;
        }
    }

    for (size_t i = 0; i < tx.body.vKernels.size(); i++)
    {
        const CMWKernel& kernel = tx.body.vKernels[i];
        if (kernel.IsHeightLocked() && kernel.nLockHeight > nBlockHeight)
        {
            result.error = MW_ERR_HEIGHT_LOCKED;
            result.nKernelIndex = (int)i;
            return result;
        }
    }

    return result;
}

bool CMWOutput::Verify() const
{
    if (IsNull())
        return false;

    if (rangeProof.IsNull())
        return false;

    return crypto::BulletproofVerifier::Verify(commitment, rangeProof);
}

bool CMWKernel::Verify() const
{
    if (IsNull())
        return false;

    if (signature.IsNull())
        return false;

    uint256 msg = GetSignatureMessage();
    return crypto::SchnorrVerifier::VerifyExcess(excess, msg, signature);
}

bool CMWTransactionBody::Validate(const uint256& txHash) const
{

    if (!VerifyRangeProofs(*this))
    {
        printf("CMWTransactionBody::Validate() : range proof verification failed\n");
        return false;
    }

    if (!VerifyKernelSignatures(*this))
    {
        printf("CMWTransactionBody::Validate() : kernel signature verification failed\n");
        return false;
    }

    for (size_t i = 0; i < vInputs.size(); i++)
    {
        if (!vInputs[i].VerifySignature(txHash))
        {
            printf("CMWTransactionBody::Validate() : input %zu signature verification failed\n", i);
            return false;
        }
    }

    {
        std::set<Commitment> inputCommitments;
        for (size_t i = 0; i < vInputs.size(); i++)
        {
            if (!inputCommitments.insert(vInputs[i].commitment).second)
            {
                printf("CMWTransactionBody::Validate() : duplicate input commitment at %zu\n", i);
                return false;
            }
        }
    }

    {
        std::set<Commitment> outputCommitments;
        for (size_t i = 0; i < vOutputs.size(); i++)
        {
            if (!outputCommitments.insert(vOutputs[i].commitment).second)
            {
                printf("CMWTransactionBody::Validate() : duplicate output commitment at %zu\n", i);
                return false;
            }
        }
    }

    for (size_t i = 0; i < vKernels.size(); i++)
    {
        if (vKernels[i].nFee < 0)
        {
            printf("CMWTransactionBody::Validate() : kernel %zu has negative fee\n", i);
            return false;
        }
    }

    return true;
}

}

#endif
