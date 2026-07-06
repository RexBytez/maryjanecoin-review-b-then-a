#include "bridgepage.h"
#include <qt/forms/ui_bridgepage.h>

#include "walletmodel.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "wallet.h"
#include "base58.h"
#include "script.h"
#include "util.h"
#include "main.h"

#include <QMessageBox>
#include <QClipboard>
#include <QApplication>

const QString BridgePage::ESCROW_ADDRESS = "SPoto81vBGGkUxUHjxyGQEtGoUsKBQZjpKaA2uj8Gvf";

const QString POT_ESCROW_ADDRESS = "PSoLrqxK6Jia4gawpbMnkKs16tevP7pw5J";

const QString POT_TOKEN_MINT = "PotzPaFGzK3cbu1u3g5HtoZbyTUkyhka8T2aBy8LMEq";

BridgePage::BridgePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgePage),
    walletModel(0),
    clientModel(0)
{
    ui->setupUi(this);

    ui->directionStack->setCurrentIndex(0);

    connect(ui->potToSolButton, SIGNAL(clicked()), this, SLOT(onPotToSolClicked()));
    connect(ui->solToPotButton, SIGNAL(clicked()), this, SLOT(onSolToPotClicked()));

    connect(ui->initiateButton, SIGNAL(clicked()), this, SLOT(on_initiateButton_clicked()));

    connect(ui->copyEscrowButton, SIGNAL(clicked()), this, SLOT(onCopyEscrowClicked()));
    connect(ui->copyMemoButton, SIGNAL(clicked()), this, SLOT(onCopyMemoClicked()));

    connect(ui->potAddressCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onPotAddressChanged(int)));

    ui->escrowAddressEdit->setText(ESCROW_ADDRESS);
}

BridgePage::~BridgePage()
{
    delete ui;
}

void BridgePage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if (model) {

        populatePotAddresses();
    }
}

void BridgePage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void BridgePage::populatePotAddresses()
{
    if (!walletModel || !walletModel->getWallet()) {
        return;
    }

    ui->potAddressCombo->clear();

    CWallet *wallet = walletModel->getWallet();
    LOCK(wallet->cs_wallet);

    BOOST_FOREACH(const PAIRTYPE(CTxDestination, std::string)& item, wallet->mapAddressBook) {
        const CBitcoinAddress& address = item.first;
        const std::string& strName = item.second;

        if (IsMine(*wallet, address.Get())) {
            QString displayText = QString::fromStdString(address.ToString());
            if (!strName.empty()) {
                displayText = QString::fromStdString(strName) + " (" + displayText + ")";
            }
            ui->potAddressCombo->addItem(displayText, QString::fromStdString(address.ToString()));
        }
    }

    if (ui->potAddressCombo->count() > 0) {
        onPotAddressChanged(0);
    }
}

void BridgePage::onPotToSolClicked()
{
    ui->directionStack->setCurrentIndex(0);
}

void BridgePage::onSolToPotClicked()
{
    ui->directionStack->setCurrentIndex(1);
}

void BridgePage::onCopyEscrowClicked()
{
    QApplication::clipboard()->setText(ui->escrowAddressEdit->text());
    QMessageBox::information(this, tr("Copied"), tr("Escrow address copied to clipboard"));
}

void BridgePage::onCopyMemoClicked()
{
    QString memo = ui->memoEdit->text();
    if (!memo.isEmpty()) {
        QApplication::clipboard()->setText(memo);
        QMessageBox::information(this, tr("Copied"), tr("Memo copied to clipboard"));
    }
}

void BridgePage::onPotAddressChanged(int index)
{
    if (index < 0) return;

    QString address = ui->potAddressCombo->itemData(index).toString();
    if (!address.isEmpty()) {
        ui->memoEdit->setText("MARYJ:" + address);
    }
}

void BridgePage::on_initiateButton_clicked()
{
    if (!walletModel) {
        showErrorMessage(tr("Bridge Error"), tr("Wallet not available"));
        return;
    }

    if (!validateInputs()) {
        return;
    }

    QString solanaAddress = ui->destinationAddressEdit->text().trimmed();
    double amount = ui->amountEdit->text().toDouble();

    QString txid = createMaryJaneCoinTransaction(solanaAddress, amount);

    if (!txid.isEmpty()) {
        showSuccessMessage(tr("Transfer Initiated"),
            tr("Transaction sent successfully!\n\n"
               "Transaction ID:\n%1\n\n"
               "Amount: %2 MARYJ\n"
               "Destination: %3\n\n"
               "The bridge will send SPL MARYJ to your Solana address after 10 confirmations.\n\n"
               "Fee: 0.5%% or $4.20 minimum (auto-sold for SOL escrow gas).")
               .arg(txid)
               .arg(QString::number(amount, 'f', 2))
               .arg(solanaAddress));

        ui->amountEdit->clear();
        ui->destinationAddressEdit->clear();
    }
}

bool BridgePage::validateInputs()
{

    QString amountStr = ui->amountEdit->text();
    bool ok;
    double amount = amountStr.toDouble(&ok);

    if (!ok || amount <= 0) {
        showErrorMessage(tr("Invalid Input"), tr("Please enter a valid amount"));
        return false;
    }

    if (amount < 100.0) {
        showErrorMessage(tr("Amount Too Small"), tr("Minimum transfer amount is 100 MARYJ"));
        return false;
    }

    QString solanaAddress = ui->destinationAddressEdit->text().trimmed();
    if (solanaAddress.isEmpty()) {
        showErrorMessage(tr("Invalid Address"), tr("Please enter a Solana address"));
        return false;
    }

    if (solanaAddress.length() < 32 || solanaAddress.length() > 44) {
        showErrorMessage(tr("Invalid Solana Address"),
            tr("Solana addresses are typically 32-44 characters long"));
        return false;
    }

    QRegExp base58Regex("^[1-9A-HJ-NP-Za-km-z]+$");
    if (!base58Regex.exactMatch(solanaAddress)) {
        showErrorMessage(tr("Invalid Solana Address"),
            tr("Solana address contains invalid characters"));
        return false;
    }

    return true;
}

QString BridgePage::createMaryJaneCoinTransaction(const QString &solanaAddress, double amount)
{
    if (!walletModel) {
        return QString();
    }

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid()) {
        showErrorMessage(tr("Wallet Locked"), tr("Please unlock your wallet to send transactions"));
        return QString();
    }

    int64_t nAmount = amount * COIN;

    CBitcoinAddress address(POT_ESCROW_ADDRESS.toStdString());
    if (!address.IsValid()) {
        showErrorMessage(tr("Configuration Error"), tr("Invalid escrow address"));
        return QString();
    }

    CScript scriptPubKey;
    scriptPubKey.SetDestination(address.Get());

    std::string opReturnData = "SOL:" + solanaAddress.toStdString();
    CScript opReturnScript;
    opReturnScript << OP_RETURN << std::vector<unsigned char>(opReturnData.begin(), opReturnData.end());

    CWalletTx wtx;

    std::vector<std::pair<CScript, int64_t> > vecSend;
    vecSend.push_back(std::make_pair(scriptPubKey, nAmount));
    vecSend.push_back(std::make_pair(opReturnScript, 0));

    CReserveKey reservekey(walletModel->getWallet());
    int64_t nFeeRequired = 0;
    int nSplitBlock = 1;

    if (!walletModel->getWallet()->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nSplitBlock, NULL)) {
        showErrorMessage(tr("Transaction Error"), tr("Failed to create transaction. Check your balance."));
        return QString();
    }

    if (!walletModel->getWallet()->CommitTransaction(wtx, reservekey)) {
        showErrorMessage(tr("Transaction Error"), tr("Failed to broadcast transaction"));
        return QString();
    }

    return QString::fromStdString(wtx.GetHash().GetHex());
}

void BridgePage::showErrorMessage(const QString &title, const QString &message)
{
    QMessageBox::warning(this, title, message);
}

void BridgePage::showSuccessMessage(const QString &title, const QString &message)
{
    QMessageBox::information(this, title, message);
}
