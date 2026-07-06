#ifdef ENABLE_MWEB

#include "mw/state/mmr.h"

#include <cassert>
#include <cstring>

namespace mw {

namespace {

int BitLength(uint64_t n)
{
    int len = 0;
    while (n > 0)
    {
        len++;
        n >>= 1;
    }
    return len;
}

uint64_t AllOnesBelow(uint64_t n)
{

    if (n == 0) return 0;
    int bits = BitLength(n);
    uint64_t allOnes = (1ULL << bits) - 1;
    if (allOnes <= n)
        return allOnes;
    return (1ULL << (bits - 1)) - 1;
}

uint64_t LeafIndexToPos(uint64_t nLeafIndex)
{
    uint64_t bits = nLeafIndex;
    int popcount = 0;
    while (bits > 0)
    {
        popcount += (int)(bits & 1);
        bits >>= 1;
    }
    return 2 * nLeafIndex - (uint64_t)popcount;
}

}

int CMMR::GetHeight(uint64_t nPos)
{
    uint64_t n = nPos + 1;

    while (n > 0)
    {
        uint64_t peakSize = AllOnesBelow(n);
        if (n == peakSize)
        {

            return BitLength(n) - 1;
        }

        n -= peakSize;
    }

    return 0;
}

bool CMMR::IsLeftSibling(uint64_t nPos)
{
    int h = GetHeight(nPos);
    uint64_t siblingOffset = (1ULL << (h + 1)) - 1;

    return GetHeight(nPos + siblingOffset) == h;
}

uint64_t CMMR::Append(const uint256& hash)
{
    uint64_t leafIndex = nLeaves;

    uint256 leafHash = HashLeaf(hash);

    vNodes.push_back(leafHash);
    nLeaves++;

    int height = 0;
    uint64_t idx = leafIndex;

    while ((idx & 1) == 1)
    {

        uint64_t subtreeSize = (1ULL << (height + 1)) - 1;
        size_t rightIdx = vNodes.size() - 1;
        size_t leftIdx = rightIdx - subtreeSize;

        uint256 parent = HashParent(vNodes[leftIdx], vNodes[rightIdx]);
        vNodes.push_back(parent);

        height++;
        idx >>= 1;
    }

    return leafIndex;
}

uint64_t CMMR::NodeCountForLeaves(uint64_t nTargetLeaves)
{
    uint64_t bits = nTargetLeaves;
    uint64_t popcount = 0;
    while (bits > 0)
    {
        popcount += (bits & 1);
        bits >>= 1;
    }
    return 2 * nTargetLeaves - popcount;
}

bool CMMR::Rewind(uint64_t nTargetLeaves)
{

    if (nTargetLeaves > nLeaves)
        return false;

    if (nTargetLeaves == nLeaves)
        return true;

    uint64_t nTargetNodes = NodeCountForLeaves(nTargetLeaves);

    if (nTargetNodes > vNodes.size())
        return false;

    vNodes.resize(static_cast<size_t>(nTargetNodes));
    nLeaves = nTargetLeaves;
    return true;
}

std::vector<uint64_t> CMMR::GetPeaks() const
{
    std::vector<uint64_t> peaks;
    if (nLeaves == 0)
        return peaks;

    uint64_t remaining = nLeaves;
    uint64_t nodeOffset = 0;

    for (int h = 63; h >= 0; h--)
    {
        if (remaining >= (1ULL << h))
        {
            uint64_t treeSize = (1ULL << (h + 1)) - 1;
            peaks.push_back(nodeOffset + treeSize - 1);
            nodeOffset += treeSize;
            remaining -= (1ULL << h);
        }
    }

    return peaks;
}

uint256 CMMR::GetRoot() const
{
    if (nLeaves == 0)
        return uint256(0);

    std::vector<uint64_t> peaks = GetPeaks();

    if (peaks.empty())
        return uint256(0);

    if (peaks.size() == 1)
    {
        if (peaks[0] < vNodes.size())
            return vNodes[peaks[0]];
        return uint256(0);
    }

    uint256 bag;
    if (peaks.back() < vNodes.size())
        bag = vNodes[peaks.back()];
    else
        return uint256(0);

    for (int i = (int)peaks.size() - 2; i >= 0; i--)
    {
        if (peaks[i] < vNodes.size())
            bag = HashParent(vNodes[peaks[i]], bag);
        else
            return uint256(0);
    }

    return bag;
}

CMMRProof CMMR::GetProof(uint64_t nLeafIndex) const
{
    CMMRProof proof;
    proof.nIndex = nLeafIndex;

    if (nLeafIndex >= nLeaves)
        return proof;

    uint64_t pos = LeafIndexToPos(nLeafIndex);

    if (pos >= vNodes.size())
        return proof;

    std::vector<uint64_t> peaks = GetPeaks();

    int myPeakIdx = -1;
    {
        uint64_t remaining = nLeaves;
        uint64_t leafOffset = 0;
        int peakCounter = 0;
        for (int h = 63; h >= 0; h--)
        {
            if (remaining >= (1ULL << h))
            {
                uint64_t treeLeaves = (1ULL << h);
                if (nLeafIndex >= leafOffset && nLeafIndex < leafOffset + treeLeaves)
                {
                    myPeakIdx = peakCounter;
                    break;
                }
                leafOffset += treeLeaves;
                remaining -= (1ULL << h);
                peakCounter++;
            }
        }
    }

    uint64_t currentPos = pos;
    while (true)
    {

        bool isPeak = false;
        for (size_t pi = 0; pi < peaks.size(); pi++)
        {
            if (peaks[pi] == currentPos)
            {
                isPeak = true;
                break;
            }
        }
        if (isPeak)
            break;

        int h = GetHeight(currentPos);
        bool isLeft = IsLeftSibling(currentPos);
        uint64_t siblingOffset = (1ULL << (h + 1)) - 1;

        uint64_t siblingPos;
        uint64_t parentPos;

        if (isLeft)
        {

            siblingPos = currentPos + siblingOffset;

            parentPos = siblingPos + 1;
        }
        else
        {

            siblingPos = currentPos - siblingOffset;

            parentPos = currentPos + 1;
        }

        if (siblingPos >= vNodes.size())
            break;

        proof.vBranch.push_back(vNodes[siblingPos]);
        proof.vSide.push_back(!isLeft);

        currentPos = parentPos;
    }

    if (peaks.size() > 1 && myPeakIdx >= 0)
    {

        for (int i = (int)peaks.size() - 1; i > myPeakIdx; i--)
        {
            if (peaks[i] < vNodes.size())
            {
                proof.vBranch.push_back(vNodes[peaks[i]]);
                proof.vSide.push_back(false);
            }
        }

        for (int i = myPeakIdx - 1; i >= 0; i--)
        {
            if (peaks[i] < vNodes.size())
            {
                proof.vBranch.push_back(vNodes[peaks[i]]);
                proof.vSide.push_back(true);
            }
        }
    }

    return proof;
}

bool CMMR::VerifyProof(const uint256& root,
                        const uint256& leafHash,
                        const CMMRProof& proof)
{
    if (proof.vBranch.size() != proof.vSide.size())
        return false;

    uint256 current = HashLeaf(leafHash);

    for (size_t i = 0; i < proof.vBranch.size(); i++)
    {
        if (proof.vSide[i])
        {

            current = HashParent(proof.vBranch[i], current);
        }
        else
        {

            current = HashParent(current, proof.vBranch[i]);
        }
    }

    return current == root;
}

uint256 CMMR::GetLeafHash(uint64_t nLeafIndex) const
{
    if (nLeafIndex >= nLeaves)
        return uint256(0);

    uint64_t pos = LeafIndexToPos(nLeafIndex);

    if (pos >= vNodes.size())
        return uint256(0);

    return vNodes[pos];
}

uint64_t CMMR::GetNodeIndex(uint64_t nLeafIndex, int nHeight) const
{
    if (nHeight == 0)
        return LeafIndexToPos(nLeafIndex);

    uint64_t pos = LeafIndexToPos(nLeafIndex);
    for (int h = 0; h < nHeight; h++)
    {
        bool isLeft = IsLeftSibling(pos);
        uint64_t siblingOffset = (1ULL << (h + 1)) - 1;
        if (isLeft)
        {

            pos = pos + siblingOffset + 1;
        }
        else
        {

            pos = pos + 1;
        }
    }
    return pos;
}

}

#endif
