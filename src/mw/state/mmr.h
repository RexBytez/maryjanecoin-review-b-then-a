#ifndef MARYJANECOIN_MW_STATE_MMR_H
#define MARYJANECOIN_MW_STATE_MMR_H

#ifdef ENABLE_MWEB

#include "mw/mw_common.h"

#include <vector>
#include <stdint.h>

#include <openssl/sha.h>

namespace mw {

struct CMMRProof
{
    uint64_t nIndex;
    std::vector<uint256> vBranch;
    mutable std::vector<bool> vSide;

    CMMRProof() : nIndex(0) {}

    bool IsNull() const { return vBranch.empty(); }

    std::vector<unsigned char> EncodeSide() const
    {
        std::vector<unsigned char> bits;
        for (size_t i = 0; i < vSide.size(); i += 8)
        {
            unsigned char byte = 0;
            for (size_t j = 0; j < 8 && i + j < vSide.size(); j++)
            {
                if (vSide[i + j]) byte |= (1 << j);
            }
            bits.push_back(byte);
        }
        return bits;
    }

    void DecodeSide(const std::vector<unsigned char>& bits, size_t nBranchSize) const
    {
        vSide.clear();
        for (size_t i = 0; i < bits.size(); i++)
        {
            for (size_t j = 0; j < 8; j++)
            {
                vSide.push_back((bits[i] >> j) & 1);
            }
        }
        vSide.resize(nBranchSize);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(nIndex);
        READWRITE(vBranch);
        if (fWrite || fGetSize)
        {
            std::vector<unsigned char> bits = const_cast<CMMRProof*>(this)->EncodeSide();
            READWRITE(bits);
        }
        if (fRead)
        {
            std::vector<unsigned char> bits;
            READWRITE(bits);
            const_cast<CMMRProof*>(this)->DecodeSide(bits, vBranch.size());
        }
    )
};

class CMMR
{
private:

    std::vector<uint256> vNodes;

    uint64_t nLeaves;

    static uint256 HashParent(const uint256& left, const uint256& right)
    {
        uint256 hash;
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        const char* prefix = "MARYJ_MMR_NODE";
        SHA256_Update(&ctx, prefix, 14);
        SHA256_Update(&ctx, left.begin(), 32);
        SHA256_Update(&ctx, right.begin(), 32);
        SHA256_Final((unsigned char*)&hash, &ctx);

        uint256 hash2;
        SHA256((unsigned char*)&hash, sizeof(hash), (unsigned char*)&hash2);
        return hash2;
    }

    static uint256 HashLeaf(const uint256& leaf)
    {
        uint256 hash;
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        const char* prefix = "MARYJ_MMR_LEAF";
        SHA256_Update(&ctx, prefix, 14);
        SHA256_Update(&ctx, leaf.begin(), 32);
        SHA256_Final((unsigned char*)&hash, &ctx);

        uint256 hash2;
        SHA256((unsigned char*)&hash, sizeof(hash), (unsigned char*)&hash2);
        return hash2;
    }

    uint64_t GetNodeIndex(uint64_t nLeafIndex, int nHeight) const;

    std::vector<uint64_t> GetPeaks() const;

    static int GetHeight(uint64_t nPos);

    static bool IsLeftSibling(uint64_t nPos);

public:
    CMMR() : nLeaves(0) {}

    uint64_t GetSize() const { return nLeaves; }

    bool IsEmpty() const { return nLeaves == 0; }

    uint64_t Append(const uint256& hash);

    bool Rewind(uint64_t nTargetLeaves);

    static uint64_t NodeCountForLeaves(uint64_t nTargetLeaves);

    uint256 GetRoot() const;

    CMMRProof GetProof(uint64_t nLeafIndex) const;

    static bool VerifyProof(const uint256& root,
                            const uint256& leafHash,
                            const CMMRProof& proof);

    uint256 GetLeafHash(uint64_t nLeafIndex) const;

    void Clear()
    {
        vNodes.clear();
        nLeaves = 0;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vNodes);
        READWRITE(nLeaves);
    )
};

}

#endif
#endif
