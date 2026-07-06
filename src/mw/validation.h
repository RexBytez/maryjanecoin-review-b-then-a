#ifndef MARYJANECOIN_MW_VALIDATION_H
#define MARYJANECOIN_MW_VALIDATION_H

#ifdef ENABLE_MWEB

#include "mw/models/block.h"
#include "mw/models/tx_body.h"
#include "mw/state/mw_state.h"

namespace mw {

enum MWValidationError
{
    MW_OK                       = 0,
    MW_ERR_INVALID_RANGEPROOF   = 1,
    MW_ERR_INVALID_SIGNATURE    = 2,
    MW_ERR_COMMITMENT_SUM       = 3,
    MW_ERR_DOUBLE_SPEND         = 4,
    MW_ERR_OUTPUT_NOT_FOUND     = 5,
    MW_ERR_INVALID_CUTTHROUGH   = 6,
    MW_ERR_PEG_MISMATCH         = 7,
    MW_ERR_INVALID_FEE          = 8,
    MW_ERR_INVALID_KERNEL       = 9,
    MW_ERR_HEIGHT_LOCKED        = 10,
    MW_ERR_PREV_BLOCK_MISMATCH  = 11,
    MW_ERR_INFLATION            = 12,
    MW_ERR_DUPLICATE_OUTPUT     = 13,
    MW_ERR_EMPTY_BLOCK          = 14,
    MW_ERR_NONCANONICAL_ORDER   = 15,
};

struct CMWValidationResult
{
    MWValidationError error;
    std::string strMessage;
    int nKernelIndex;
    int nOutputIndex;
    int nInputIndex;

    CMWValidationResult()
        : error(MW_OK), nKernelIndex(-1), nOutputIndex(-1), nInputIndex(-1) {}

    bool IsValid() const { return error == MW_OK; }

    std::string ToString() const;
};

CMWValidationResult ValidateMWBlock(
    const CMWBlock& block,
    const CMWState& state,
    int nBlockHeight,
    int64_t nTransparentPegIn,
    int64_t nTransparentPegOut);

CMWValidationResult ValidateMWTransaction(
    const CMWTransaction& tx,
    const CMWState& state,
    int nBlockHeight);

bool VerifyKernelSignatures(const CMWTransactionBody& body);

bool VerifyRangeProofs(const CMWTransactionBody& body);

bool VerifyCommitmentSum(
    const CMWTransactionBody& body,
    const BlindingFactor& offset);

}

#endif
#endif
