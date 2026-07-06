#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <QAbstractListModel>

class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit OptionsModel(QObject *parent = 0);

    enum OptionID {
        StartAtStartup,
        MinimizeToTray,
        MapPortUPnP,
        MinimizeOnClose,
        ProxyUse,
        ProxyIP,
        ProxyPort,
        ProxySocksVersion,
        Fee,
        ReserveBalance,
        DisplayUnit,
        DisplayAddresses,
        DetachDatabases,
        Language,
        CoinControlFeatures,
        MinimizeCoinAge,
        OptionIDRowCount,
    };

    void Init();

    bool Upgrade();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    qint64 getTransactionFee();
    qint64 getReserveBalance();
    bool getMinimizeToTray();
    bool getMinimizeOnClose();
    int getDisplayUnit();
    bool getDisplayAddresses();
    bool getCoinControlFeatures();
    QString getLanguage() { return language; }

private:
    int nDisplayUnit;
    bool bDisplayAddresses;
    bool fMinimizeToTray;
    bool fMinimizeOnClose;
    bool fCoinControlFeatures;
    QString language;

Q_SIGNALS:
    void displayUnitChanged(int unit);
    void transactionFeeChanged(qint64);
    void reserveBalanceChanged(qint64);
    void coinControlFeaturesChanged(bool);
};

#endif
