#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#include <QObject>
#include <boost/signals2/connection.hpp>
#include <vector>
#include <map>

#include "support/allocators/secure.h"

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;
class CKeyID;
class CPubKey;
class COutput;
class COutPoint;
class uint256;
class CCoinControl;
class CBitcoinAddress;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class SendCoinsRecipient
{
public:
    QString address;
    QString label;
    qint64 amount;
};

class WalletModel : public QObject
{
    Q_OBJECT

public:
    explicit WalletModel(CWallet *wallet, OptionsModel *optionsModel, QObject *parent = 0);
    ~WalletModel();

    enum StatusCode
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionCreationFailed,
        TransactionCommitFailed,
        Aborted
    };

    enum EncryptionStatus
    {
        Unencrypted,
        Locked,
        Unlocked
    };

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    TransactionTableModel *getTransactionTableModel();
    CWallet *getWallet() { return wallet; }

    qint64 getBalance() const;
    qint64 getStake() const;
    qint64 getUnconfirmedBalance() const;
    qint64 getConflictedBalance() const;
    qint64 getImmatureBalance() const;
    int getNumTransactions() const;
    EncryptionStatus getEncryptionStatus() const;

    bool validateAddress(const QString &address);

    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode status=Aborted,
                         qint64 fee=0,
                         QString hex=QString()):
            status(status), fee(fee), hex(hex) {}
        StatusCode status;
        qint64 fee;
        QString hex;
    };

    SendCoinsReturn sendCoins(const QList<SendCoinsRecipient> &recipients, int nSplitBlock, const CCoinControl *coinControl=NULL);

    bool setWalletEncrypted(bool encrypted, const SecureString &passphrase);

    bool setWalletLocked(bool locked, const SecureString &passPhrase=SecureString(), bool formint=false);
    bool changePassphrase(const SecureString &oldPass, const SecureString &newPass);

    bool backupWallet(const QString &filename);

	void checkWallet(int& nMismatchSpent, int64_t& nBalanceInQuestion, int& nOrphansFound);
	void repairWallet(int& nMismatchSpent, int64_t& nBalanceInQuestion, int& nOrphansFound);

	void getStakeWeightFromValue(const int64_t nTime, const int64_t nValue, uint64_t& nWeight);
	void setSplitBlock(bool fSplitBlock);
	bool getSplitBlock();

	int getStakeForCharityPercent();
	QString getStakeForCharityAddress();

    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *wallet, bool valid, bool relock);
        ~UnlockContext();

        bool isValid() const { return valid; }

        UnlockContext(const UnlockContext& obj) { CopyFrom(obj); }
        UnlockContext& operator=(const UnlockContext& rhs) { CopyFrom(rhs); return *this; }
    private:
        WalletModel *wallet;
        bool valid;
        mutable bool relock;

        void CopyFrom(const UnlockContext& rhs);
    };

    UnlockContext requestUnlock();

    bool getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    void getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs);
    void listCoins(std::map<QString, std::vector<COutput> >& mapCoins) const;
    bool isLockedCoin(uint256 hash, unsigned int n) const;
    void lockCoin(COutPoint& output);
    void unlockCoin(COutPoint& output);
    void listLockedCoins(std::vector<COutPoint>& vOutpts);
	bool isMine(const CBitcoinAddress &address);

private:
    CWallet *wallet;

    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;

    qint64 cachedBalance;
    qint64 cachedStake;
    qint64 cachedUnconfirmedBalance;
    qint64 cachedConflictedBalance;
    qint64 cachedImmatureBalance;
    qint64 cachedNumTransactions;
    EncryptionStatus cachedEncryptionStatus;
    int cachedNumBlocks;

    QTimer *pollTimer;

    boost::signals2::connection statusChangedConnection;
    boost::signals2::connection addressBookChangedConnection;
    boost::signals2::connection transactionChangedConnection;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    void checkBalanceChanged();

public Q_SLOTS:

    void updateStatus();

    void updateTransaction(const QString &hash, int status);

    void updateAddressBook(const QString &address, const QString &label, bool isMine, int status);

    void pollBalanceChanged();

Q_SIGNALS:

    void balanceChanged(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 conflictedBalance, qint64 immatureBalance);

    void numTransactionsChanged(int count);

    void encryptionStatusChanged(int status);

    void requireUnlock();

    void error(const QString &title, const QString &message, bool modal);
};

#endif
