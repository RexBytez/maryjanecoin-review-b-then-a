#ifndef MARYJANECOIN_MW_PEG_H
#define MARYJANECOIN_MW_PEG_H

#ifdef ENABLE_MWEB

#include "mw/models/tx_body.h"
#include "mw/models/kernel.h"
#include "main.h"
#include "wallet.h"

namespace mw {

struct CPegInResult
{
    bool fSuccess;
    std::string strError;

    CMWOutput mwOutput;

    CMWKernel kernel;

    CTransaction transparentTx;

    BlindingFactor outputBlind;

    CPegInResult() : fSuccess(false) {}
};

struct CPegOutResult
{
    bool fSuccess;
    std::string strError;

    CMWInput mwInput;

    CMWKernel kernel;

    CScript destScript;

    BlindingFactor inputBlind;

    CPegOutResult() : fSuccess(false) {}
};

CPegInResult CreatePegIn(CWallet* pWallet, int64_t nAmount);

CPegOutResult CreatePegOut(CWallet* pWallet, int64_t nAmount,
                           const CTxDestination& dest);

bool IsPegInTransaction(const CTransaction& tx, Commitment& commitmentOut,
                        int64_t& nAmountOut);

bool ValidatePegOuts(const std::vector<CMWKernel>& vKernels,
                     const std::vector<CTxOut>& vTransparentOutputs);

CScript GetPegInMarkerScript(const Commitment& commitment);

bool ExtractPegInCommitment(const CScript& script, Commitment& commitmentOut);

}

#endif
#endif
