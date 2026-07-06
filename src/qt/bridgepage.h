#ifndef BITCOIN_QT_BRIDGEPAGE_H
#define BITCOIN_QT_BRIDGEPAGE_H

#include <QWidget>

namespace Ui {
    class BridgePage;
}

class WalletModel;
class ClientModel;

class BridgePage : public QWidget
{
    Q_OBJECT

public:
    explicit BridgePage(QWidget *parent = 0);
    ~BridgePage();

    void setWalletModel(WalletModel *model);
    void setClientModel(ClientModel *model);

private Q_SLOTS:
    void on_initiateButton_clicked();
    void onPotToSolClicked();
    void onSolToPotClicked();
    void onCopyEscrowClicked();
    void onCopyMemoClicked();
    void onPotAddressChanged(int index);

private:
    Ui::BridgePage *ui;
    WalletModel *walletModel;
    ClientModel *clientModel;

    static const QString ESCROW_ADDRESS;

    void populatePotAddresses();
    bool validateInputs();
    QString createMaryJaneCoinTransaction(const QString &solanaAddress, double amount);
    void showErrorMessage(const QString &title, const QString &message);
    void showSuccessMessage(const QString &title, const QString &message);
};

#endif
