#ifndef BITCOINGUI_H
#define BITCOINGUI_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <QMainWindow>
#include <QPushButton>
#include <QSystemTrayIcon>

#include "util.h"

class TransactionTableModel;
class ClientModel;
class WalletModel;
class TransactionView;
class OverviewPage;
class StatisticsPage;
class BlockBrowser;
class ChatWindow;
class BridgePage;
class AddressBookPage;
class SendCoinsDialog;
class SignVerifyMessageDialog;
class StakeForCharityDialog;
class Notificator;
class RPCConsole;
class WalletModel;
class TransactionTableModel;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QTableView;
class QAbstractItemModel;
class QModelIndex;
class QProgressBar;
class QStackedWidget;
class QToolBar;
class QUrl;
QT_END_NAMESPACE

class BitcoinGUI : public QMainWindow
{
    Q_OBJECT
public:
    explicit BitcoinGUI(QWidget *parent = 0);
    ~BitcoinGUI();

    void setClientModel(ClientModel *clientModel);

    void setWalletModel(WalletModel *walletModel);

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;

    QStackedWidget *centralWidget;

    OverviewPage *overviewPage;
	StatisticsPage *statisticsPage;
	BlockBrowser *blockBrowser;
	ChatWindow *chatWindow;
	BridgePage *bridgePage;
    QWidget *transactionsPage;
    AddressBookPage *addressBookPage;
    AddressBookPage *receiveCoinsPage;
    SendCoinsDialog *sendCoinsPage;
    SignVerifyMessageDialog *signVerifyMessageDialog;
    StakeForCharityDialog *stakeForCharityDialog;

	QPushButton *historyCopyButton;
	QPushButton *historyExportButton;
	QPushButton *historyQRButton;

    QLabel *labelEncryptionIcon;
    QLabel *labelMintingIcon;
    QLabel *labelConnectionsIcon;
    QLabel *labelBlocksIcon;
    QLabel *progressBarLabel;
    QProgressBar *progressBar;

    QMenuBar *appMenuBar;
    QToolBar *toolbar;
    QAction *overviewAction;
	QAction *statisticsAction;
	QAction *blockAction;
	QAction *chatAction;
	QAction *bridgeAction;
    QAction *historyAction;
    QAction *quitAction;
    QAction *sendCoinsAction;
    QAction *addressBookAction;
    QAction *signMessageAction;
    QAction *verifyMessageAction;
    QAction *aboutAction;
	QAction *charityAction;
    QAction *receiveCoinsAction;
    QAction *optionsAction;
    QAction *toggleHideAction;
    QAction *exportAction;
    QAction *encryptWalletAction;
	QAction *unlockWalletAction;
    QAction *backupWalletAction;
    QAction *changePassphraseAction;
    QAction *lockWalletToggleAction;
	QAction *checkWalletAction;
	QAction *repairWalletAction;
    QAction *aboutQtAction;
    QAction *openRPCConsoleAction;
	QAction *themeDefaultAction;
	QAction *themeCustomAction;
	QAction *connectionIconAction;
	QAction *stakingIconAction;
	QAction *systemDefaultThemeAction;

    QSystemTrayIcon *trayIcon;
    Notificator *notificator;
    TransactionView *transactionView;
    RPCConsole *rpcConsole;

    QMovie *syncIconMovie;

    QMovie *miningIconMovie;

    uint64_t nMinMax;
    uint64_t nWeight;
    uint64_t nNetworkWeight;
	uint64_t nHoursToMaturity;
	uint64_t nAmount;
	bool fMultiSend;
	bool fMultiSendNotify;
	int nCharityPercent;
	QString strCharityAddress;
	bool fBridgeEnabled;

    QString selectedTheme;
    QStringList themesList;

    QString themesDir;
    QAction *customActions[100];

    void createActions();

    void createMenuBar();

    void createToolBars();

    void createTrayIcon();

public Q_SLOTS:

    void setNumConnections(int count);

    void setNumBlocks(int count, int nTotalBlocks);

    void setEncryptionStatus(int status);

    void error(const QString &title, const QString &message, bool modal);

    void askFee(qint64 nFeeRequired, bool *payFee);
    void handleURI(QString strURI);

private Q_SLOTS:

    void gotoOverviewPage();

	void gotoStatisticsPage();

    void gotoBlockBrowser();
	void gotoBlockBrowserWithTx(QString txid);

	void gotoChatPage();

	void gotoBridgePage();

    void gotoHistoryPage();

    void gotoAddressBookPage();

    void gotoReceiveCoinsPage();

    void gotoSendCoinsPage();

    void gotoSignMessageTab(QString addr = "");

    void gotoVerifyMessageTab(QString addr = "");

	void lockIconClicked();

    void optionsClicked();

    void aboutClicked();
	void historySelectionChanged();
#ifndef Q_OS_MAC

    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
#endif

    void incomingTransaction(const QModelIndex & parent, int start, int end);

    void encryptWallet(bool status);

	void checkWallet();

	void repairWallet();

    void backupWallet();

    void changePassphrase();

	void lockWallet();

    void lockWalletToggle();

	void unlockWallet();

	void unlockWalletForMint();

    void showNormalIfMinimized(bool fToggleHidden = false);

    void toggleHidden();

    void updateMintingIcon();

    void updateMintingWeights();
    void charityClicked();

    void changeTheme(QString theme);
    void loadTheme(QString theme);
    void listThemes(QStringList& themes);
    void updateThemeCheckmarks();
    void keyPressEvent(QKeyEvent * e);
};

#endif
