#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include <QObject>
#include <boost/signals2/connection.hpp>

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;

QT_BEGIN_NAMESPACE
class QDateTime;
class QTimer;
QT_END_NAMESPACE

class ClientModel : public QObject
{
    Q_OBJECT
public:
    explicit ClientModel(OptionsModel *optionsModel, QObject *parent = 0);
    ~ClientModel();

    OptionsModel *getOptionsModel();

    int getNumConnections() const;
    int getNumBlocks() const;
    int getNumBlocksAtStartup();

    QDateTime getLastBlockDate() const;

    bool isTestNet() const;

    bool inInitialBlockDownload() const;

    int getNumBlocksOfPeers() const;

    QString getStatusBarWarnings() const;

    QString formatFullVersion() const;
    QString formatBuildDate() const;
    QString clientName() const;
    QString formatClientStartupTime() const;
	double GetDifficulty() const;

private:
    OptionsModel *optionsModel;

    int cachedNumBlocks;
    int cachedNumBlocksOfPeers;

    int numBlocksAtStartup;

    QTimer *pollTimer;

    boost::signals2::connection blockChangedConnection;
    boost::signals2::connection numConnectionsChangedConnection;
    boost::signals2::connection alertChangedConnection;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
Q_SIGNALS:
    void numConnectionsChanged(int count);
    void numBlocksChanged(int count, int countOfPeers);

    void error(const QString &title, const QString &message, bool modal);

public Q_SLOTS:
    void updateTimer();
    void updateNumConnections(int numConnections);
    void updateAlert(const QString &hash, int status);
};

#endif
