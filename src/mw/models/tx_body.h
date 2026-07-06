#ifndef MARYJANECOIN_MW_MODELS_TX_BODY_H
#define MARYJANECOIN_MW_MODELS_TX_BODY_H

#ifdef ENABLE_MWEB

#include "mw/models/input.h"
#include "mw/models/output.h"
#include "mw/models/kernel.h"
#include "hash.h"

#include <algorithm>
#include <set>

namespace mw {

class CMWTransactionBody
{
public:
    std::vector<CMWInput> vInputs;
    std::vector<CMWOutput> vOutputs;
    std::vector<CMWKernel> vKernels;

    CMWTransactionBody() {}

    CMWTransactionBody(const std::vector<CMWInput>& inputs,
                       const std::vector<CMWOutput>& outputs,
                       const std::vector<CMWKernel>& kernels)
        : vInputs(inputs), vOutputs(outputs), vKernels(kernels) {}

    bool IsNull() const
    {
        return vInputs.empty() && vOutputs.empty() && vKernels.empty();
    }

    void Sort()
    {
        std::sort(vInputs.begin(), vInputs.end());
        std::sort(vOutputs.begin(), vOutputs.end());
    }

    int64_t GetTotalFee() const
    {
        int64_t nTotalFee = 0;
        for (size_t i = 0; i < vKernels.size(); i++)
        {
            nTotalFee += vKernels[i].nFee;
        }
        return nTotalFee;
    }

    int64_t GetTotalPegIn() const
    {
        int64_t nTotal = 0;
        for (size_t i = 0; i < vKernels.size(); i++)
        {
            if (vKernels[i].IsPegIn())
                nTotal += vKernels[i].nPegAmount;
        }
        return nTotal;
    }

    int64_t GetTotalPegOut() const
    {
        int64_t nTotal = 0;
        for (size_t i = 0; i < vKernels.size(); i++)
        {
            if (vKernels[i].IsPegOut())
                nTotal += vKernels[i].nPegAmount;
        }
        return nTotal;
    }

    void Merge(const CMWTransactionBody& other)
    {
        vInputs.insert(vInputs.end(), other.vInputs.begin(), other.vInputs.end());
        vOutputs.insert(vOutputs.end(), other.vOutputs.begin(), other.vOutputs.end());
        vKernels.insert(vKernels.end(), other.vKernels.begin(), other.vKernels.end());
    }

    std::vector<Commitment> ApplyCutThrough()
    {
        std::vector<Commitment> vCutThrough;

        std::set<Commitment> inputCommitments;
        for (size_t i = 0; i < vInputs.size(); i++)
        {
            inputCommitments.insert(vInputs[i].commitment);
        }

        std::set<Commitment> cutSet;
        for (size_t i = 0; i < vOutputs.size(); i++)
        {
            if (inputCommitments.count(vOutputs[i].commitment) > 0)
            {
                cutSet.insert(vOutputs[i].commitment);
                vCutThrough.push_back(vOutputs[i].commitment);
            }
        }

        if (cutSet.empty())
            return vCutThrough;

        std::vector<CMWInput> newInputs;
        for (size_t i = 0; i < vInputs.size(); i++)
        {
            if (cutSet.count(vInputs[i].commitment) == 0)
                newInputs.push_back(vInputs[i]);
        }
        vInputs = newInputs;

        std::vector<CMWOutput> newOutputs;
        for (size_t i = 0; i < vOutputs.size(); i++)
        {
            if (cutSet.count(vOutputs[i].commitment) == 0)
                newOutputs.push_back(vOutputs[i]);
        }
        vOutputs = newOutputs;

        return vCutThrough;
    }

    bool Validate(const uint256& txHash) const;

    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, 0);
        ss << (int)vInputs.size() << (int)vOutputs.size() << (int)vKernels.size();
        for (size_t i = 0; i < vKernels.size(); ++i)
            ss << vKernels[i].GetSignatureMessage();
        uint256 firstHash = ss.GetHash();
        uint256 hash;
        SHA256((unsigned char*)firstHash.begin(), sizeof(uint256), (unsigned char*)hash.begin());
        return hash;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vInputs);
        READWRITE(vOutputs);
        READWRITE(vKernels);
    )
};

class CMWTransaction
{
public:
    CMWTransactionBody body;

    BlindingFactor offset;

    CMWTransaction() {}

    CMWTransaction(const CMWTransactionBody& bodyIn, const BlindingFactor& offsetIn)
        : body(bodyIn), offset(offsetIn) {}

    bool IsNull() const
    {
        return body.IsNull();
    }

    uint256 GetHash() const
    {
        return body.GetHash();
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(body);
        READWRITE(offset);
    )
};

}

#endif
#endif
