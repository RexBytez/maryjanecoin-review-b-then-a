#ifndef TRANSACTIONRECORD_H
#define TRANSACTIONRECORD_H

#include "uint256.h"

#include <QList>

class CWallet;
class CWalletTx;

class TransactionStatus
{
public:
    TransactionStatus():
            confirmed(false), sortKey(""), maturity(Mature),
            matures_in(0), status(Offline), depth(0), open_for(0), cur_num_blocks(-1)
    { }

    enum Maturity
    {
        Immature,
        Mature,
        MaturesWarning,
        NotAccepted
    };

    enum Status {
        OpenUntilDate,
        OpenUntilBlock,
        Offline,
        Unconfirmed,
        HaveConfirmations
    };

    bool confirmed;
    std::string sortKey;

    Maturity maturity;
    int matures_in;

    Status status;
    int64_t depth;
    int64_t open_for;

    int cur_num_blocks;
};

class TransactionRecord
{
public:
    enum Type
    {
        Other,
        Generated,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        RecvFromOther,
        SendToSelf,
        StakeMint
    };

    static const int RecommendedNumConfirmations = 10;

    TransactionRecord():
            hash(), time(0), type(Other), address(""), debit(0), credit(0), idx(0)
    {
    }

    TransactionRecord(uint256 hash, int64_t time):
            hash(hash), time(time), type(Other), address(""), debit(0),
            credit(0), idx(0)
    {
    }

    TransactionRecord(uint256 hash, int64_t time,
                Type type, const std::string &address,
                int64_t debit, int64_t credit):
            hash(hash), time(time), type(type), address(address), debit(debit), credit(credit),
            idx(0)
    {
    }

    static bool showTransaction(const CWalletTx &wtx);
    static QList<TransactionRecord> decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx);

    uint256 hash;
    qint64 time;
    Type type;
    std::string address;
    qint64 debit;
    qint64 credit;

    int idx;

    TransactionStatus status;

    std::string getTxID();

    void updateStatus(const CWalletTx &wtx);

    bool statusUpdateNeeded();
};

#endif
