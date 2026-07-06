#ifndef COINCONTROL_H
#define COINCONTROL_H

#include <map>
#include <set>
#include <string>
#include <vector>

static const int PRIVACY_SCORE_POOR   = 30;
static const int PRIVACY_SCORE_MEDIUM = 60;
static const int PRIVACY_SCORE_GOOD   = 80;

static const int64_t AGE_BUCKET_1DAY  = 24 * 60 * 60;
static const int64_t AGE_BUCKET_7DAY  = 7 * 24 * 60 * 60;
static const int64_t AGE_BUCKET_30DAY = 30 * 24 * 60 * 60;

class CCoinControl
{
public:
    CTxDestination destChange;
    bool fReturnChange;

    bool fPrivacyMode;
    bool fDontConsolidate;

    std::set<COutPoint> setFrozen;

    std::map<COutPoint, std::string> mapLabels;

    CCoinControl()
    {
        SetNull();
    }

    void SetNull()
    {
        destChange = CNoDestination();
        setSelected.clear();
        fReturnChange = false;
        fPrivacyMode = false;
        fDontConsolidate = false;

    }

    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }

    bool IsSelected(const uint256& hash, unsigned int n) const
    {
        COutPoint outpt(hash, n);
        return (setSelected.count(outpt) > 0);
    }

    void Select(COutPoint& output)
    {
        setSelected.insert(output);
    }

    void UnSelect(COutPoint& output)
    {
        setSelected.erase(output);
    }

    void UnSelectAll()
    {
        setSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints)
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }

    void FreezeOutput(const COutPoint& outpt)
    {
        setFrozen.insert(outpt);
    }

    void UnfreezeOutput(const COutPoint& outpt)
    {
        setFrozen.erase(outpt);
    }

    bool IsFrozen(const COutPoint& outpt) const
    {
        return (setFrozen.count(outpt) > 0);
    }

    bool IsFrozen(const uint256& hash, unsigned int n) const
    {
        COutPoint outpt(hash, n);
        return (setFrozen.count(outpt) > 0);
    }

    void ListFrozen(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setFrozen.begin(), setFrozen.end());
    }

    int GetFrozenCount() const
    {
        return (int)setFrozen.size();
    }

    void SetLabel(const COutPoint& outpt, const std::string& strLabel)
    {
        if (strLabel.empty())
            mapLabels.erase(outpt);
        else
            mapLabels[outpt] = strLabel;
    }

    std::string GetLabel(const COutPoint& outpt) const
    {
        std::map<COutPoint, std::string>::const_iterator it = mapLabels.find(outpt);
        if (it != mapLabels.end())
            return it->second;
        return "";
    }

    bool HasLabel(const COutPoint& outpt) const
    {
        return (mapLabels.count(outpt) > 0);
    }

    static int GetPrivacyScore(int nDepth)
    {
        if (nDepth <= 0) return 0;
        if (nDepth < 6)  return 20;
        if (nDepth < 20) return 40;
        if (nDepth < 100) return 60;
        if (nDepth < 500) return 80;
        return 100;
    }

    static int GetAgeBucket(int64_t nAgeSecs)
    {
        if (nAgeSecs < AGE_BUCKET_1DAY)  return 0;
        if (nAgeSecs < AGE_BUCKET_7DAY)  return 1;
        if (nAgeSecs < AGE_BUCKET_30DAY) return 2;
        return 3;
    }

private:
    std::set<COutPoint> setSelected;

};

#endif
