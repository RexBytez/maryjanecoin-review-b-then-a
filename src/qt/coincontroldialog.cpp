#include "coincontroldialog.h"
#include <qt/forms/ui_coincontroldialog.h>

#include "init.h"
#include "bitcoinunits.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "optionsmodel.h"
#include "coincontrol.h"
#include "qcomboboxfiltercoins.h"
#include "bitcoinrpc.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QCursor>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFlags>
#include <QIcon>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>

using namespace std;
QList<qint64> CoinControlDialog::payAmounts;
CCoinControl* CoinControlDialog::coinControl = new CCoinControl();

CoinControlDialog::CoinControlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CoinControlDialog),
    model(0)
{
    ui->setupUi(this);

    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
             copyTransactionHashAction = new QAction(tr("Copy transaction ID"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTransactionHashAction);

    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTransactionHashAction, SIGNAL(triggered()), this, SLOT(copyTransactionHash()));

    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);

    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(clipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(clipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(clipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(clipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(clipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(clipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(clipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(clipboardChange()));

    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    connect(ui->radioTreeMode, SIGNAL(toggled(bool)), this, SLOT(radioTreeMode(bool)));
    connect(ui->radioListMode, SIGNAL(toggled(bool)), this, SLOT(radioListMode(bool)));

    connect(ui->treeWidget, SIGNAL(itemChanged( QTreeWidgetItem*, int)), this, SLOT(viewItemChanged( QTreeWidgetItem*, int)));

    ui->treeWidget->header()->setSectionsClickable(true);
    connect(ui->treeWidget->header(), SIGNAL(sectionClicked(int)), this, SLOT(headerSectionClicked(int)));

    connect(ui->buttonBox, SIGNAL(clicked( QAbstractButton*)), this, SLOT(buttonBoxClicked(QAbstractButton*)));

    connect(ui->pushButtonSelectAll, SIGNAL(clicked()), this, SLOT(buttonSelectAllClicked()));

    connect(ui->pushButtonCustomCC, SIGNAL(clicked()), this, SLOT(customSelectCoins()));

    ui->treeWidget->setColumnWidth(COLUMN_CHECKBOX, 45);
    ui->treeWidget->setColumnWidth(COLUMN_AMOUNT, 100);
	ui->treeWidget->setColumnWidth(COLUMN_CONFIRMATIONS, 85);
	ui->treeWidget->setColumnWidth(COLUMN_AGE, 55);
	ui->treeWidget->setColumnWidth(COLUMN_TIMEESTIMATE, 110);
	ui->treeWidget->setColumnWidth(COLUMN_WEIGHT, 75);
    ui->treeWidget->setColumnWidth(COLUMN_LABEL, 125);
    ui->treeWidget->setColumnWidth(COLUMN_ADDRESS, 275);
    ui->treeWidget->setColumnWidth(COLUMN_DATE, 110);
    ui->treeWidget->setColumnWidth(COLUMN_PRIORITY, 100);
	ui->treeWidget->setColumnHidden(COLUMN_AGE_INT64, true);
    ui->treeWidget->setColumnHidden(COLUMN_TXHASH, true);
    ui->treeWidget->setColumnHidden(COLUMN_VOUT_INDEX, true);
    ui->treeWidget->setColumnHidden(COLUMN_AMOUNT_INT64, true);
    ui->treeWidget->setColumnHidden(COLUMN_PRIORITY_INT64, true);

    sortView(COLUMN_CONFIRMATIONS, Qt::DescendingOrder);

	ui->QComboBoxFilterCoins->addItem("< Amount");
	ui->QComboBoxFilterCoins->addItem("> Amount");
	ui->QComboBoxFilterCoins->addItem("< Weight");
	ui->QComboBoxFilterCoins->addItem("> Weight");
	ui->QComboBoxFilterCoins->addItem("> Age");
	ui->QComboBoxFilterCoins->addItem("< Age");
}

CoinControlDialog::~CoinControlDialog()
{
    delete ui;
}

void CoinControlDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel() && model->getAddressTableModel())
    {
        updateView();

        CoinControlDialog::updateLabels(model, this);
    }
}

QString CoinControlDialog::strPad(QString s, int nPadLength, QString sPadding)
{
    while (s.length() < nPadLength)
        s = sPadding + s;

    return s;
}

void CoinControlDialog::buttonBoxClicked(QAbstractButton* button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
        done(QDialog::Accepted);
}

void CoinControlDialog::buttonSelectAllClicked()
{
    Qt::CheckState state = Qt::Checked;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
    {
        if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) != Qt::Unchecked)
        {
            state = Qt::Unchecked;
        }
		coinControl->UnSelectAll();
    }
    ui->treeWidget->setEnabled(false);
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
            if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) != state)
                ui->treeWidget->topLevelItem(i)->setCheckState(COLUMN_CHECKBOX, state);
    ui->treeWidget->setEnabled(true);
    CoinControlDialog::updateLabels(model, this);
}

void CoinControlDialog::customSelectCoins()
{
	QString strUserAmount = ui->lineEditCustomCC->text();
    QString strComboText = ui->QComboBoxFilterCoins->currentText();

	double dUserAmount = QString(strUserAmount).toDouble();
	bool treeMode = ui->radioTreeMode->isChecked();

		QFlags<Qt::ItemFlag> flgCheckbox=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;

		map<QString, vector<COutput> > mapCoins;
		model->listCoins(mapCoins);

		BOOST_FOREACH(PAIRTYPE(QString, vector<COutput>) coins, mapCoins)
		{
			QTreeWidgetItem *itemWalletAddress = new QTreeWidgetItem();

			QTreeWidgetItem *itemOutput;
			if (treeMode)    itemOutput = new QTreeWidgetItem(itemWalletAddress);
			else             itemOutput = new QTreeWidgetItem(ui->treeWidget);
			itemOutput->setFlags(flgCheckbox);
			itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);
			BOOST_FOREACH(const COutput& out, coins.second)
			{

				uint256 txhash = out.tx->GetHash();

				double dCoinAmount = out.tx->vout[out.i].nValue;

                uint64_t nTxWeight = 0;
				model->getStakeWeightFromValue(out.tx->GetTxTime(), out.tx->vout[out.i].nValue, nTxWeight);

				double dAge = (GetTime() - out.tx->GetTxTime()) / (double)(1440 * 60);

				COutPoint outpt(txhash, out.i);

				if (strComboText == "< Amount")
				{
					if (dCoinAmount < dUserAmount * COIN)
					{
						coinControl->Select(outpt);
						itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
					}
				}
				else if (strComboText == "> Amount")
				{
					if (dCoinAmount > dUserAmount * COIN)
					{
						coinControl->Select(outpt);
						itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
					}
				}
				else if (strComboText == "< Weight")
				{
					if (nTxWeight < dUserAmount)
					{
						coinControl->Select(outpt);
						itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
					}
				}
				else if (strComboText == "> Weight")
				{
					if (nTxWeight > dUserAmount)
					{
						coinControl->Select(outpt);
						itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
					}
				}
				else if (strComboText == "< Age")
				{
					if (dAge < dUserAmount)
					{
						coinControl->Select(outpt);
						itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
					}
				}
				else if (strComboText == "> Age")
				{
					if (dAge > dUserAmount)
					{
						coinControl->Select(outpt);
						itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
					}
				}
				else
				{
					coinControl->UnSelect(outpt);
					itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);
				}
			}
		}
	CoinControlDialog::updateLabels(model, this);
	updateView();
}

void CoinControlDialog::showMenu(const QPoint &point)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(point);
    if(item)
    {
        contextMenuItem = item;

        if (item->text(COLUMN_TXHASH).length() == 64)
        {
            copyTransactionHashAction->setEnabled(true);

        }
        else
        {
            copyTransactionHashAction->setEnabled(false);

        }

        contextMenu->exec(QCursor::pos());
    }
}

void CoinControlDialog::copyAmount()
{
    QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_AMOUNT));
}

void CoinControlDialog::copyLabel()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_LABEL).length() == 0 && contextMenuItem->parent())
        QApplication::clipboard()->setText(contextMenuItem->parent()->text(COLUMN_LABEL));
    else
        QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_LABEL));
}

void CoinControlDialog::copyAddress()
{
    if (ui->radioTreeMode->isChecked() && contextMenuItem->text(COLUMN_ADDRESS).length() == 0 && contextMenuItem->parent())
        QApplication::clipboard()->setText(contextMenuItem->parent()->text(COLUMN_ADDRESS));
    else
        QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_ADDRESS));
}

void CoinControlDialog::copyTransactionHash()
{
    QApplication::clipboard()->setText(contextMenuItem->text(COLUMN_TXHASH));
}

void CoinControlDialog::clipboardQuantity()
{
    QApplication::clipboard()->setText(ui->labelCoinControlQuantity->text());
}

void CoinControlDialog::clipboardAmount()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

void CoinControlDialog::clipboardFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
}

void CoinControlDialog::clipboardAfterFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
}

void CoinControlDialog::clipboardBytes()
{
    QApplication::clipboard()->setText(ui->labelCoinControlBytes->text());
}

void CoinControlDialog::clipboardPriority()
{
    QApplication::clipboard()->setText(ui->labelCoinControlPriority->text());
}

void CoinControlDialog::clipboardLowOutput()
{
    QApplication::clipboard()->setText(ui->labelCoinControlLowOutput->text());
}

void CoinControlDialog::clipboardChange()
{
    QApplication::clipboard()->setText(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")));
}

void CoinControlDialog::sortView(int column, Qt::SortOrder order)
{
    sortColumn = column;
    sortOrder = order;
    ui->treeWidget->sortItems(column, order);
    ui->treeWidget->header()->setSortIndicator((sortColumn == COLUMN_AMOUNT_INT64 ? COLUMN_AMOUNT : (sortColumn == COLUMN_PRIORITY_INT64 ? COLUMN_PRIORITY : sortColumn)), sortOrder);
}

void CoinControlDialog::headerSectionClicked(int logicalIndex)
{
    if (logicalIndex == COLUMN_CHECKBOX)
    {
        ui->treeWidget->header()->setSortIndicator((sortColumn == COLUMN_AMOUNT_INT64 ? COLUMN_AMOUNT : (sortColumn == COLUMN_PRIORITY_INT64 ? COLUMN_PRIORITY : sortColumn)), sortOrder);
    }
    else
    {
        if (logicalIndex == COLUMN_AMOUNT)
            logicalIndex = COLUMN_AMOUNT_INT64;

		if (logicalIndex == COLUMN_AGE)
            logicalIndex = COLUMN_AGE_INT64;

        if (logicalIndex == COLUMN_PRIORITY)
            logicalIndex = COLUMN_PRIORITY_INT64;

        if (sortColumn == logicalIndex)
            sortOrder = ((sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder);
        else
        {
            sortColumn = logicalIndex;
            sortOrder = ((sortColumn == COLUMN_AMOUNT_INT64 || sortColumn == COLUMN_PRIORITY_INT64 || sortColumn == COLUMN_DATE || sortColumn == COLUMN_CONFIRMATIONS || sortColumn == COLUMN_AGE_INT64) ? Qt::DescendingOrder : Qt::AscendingOrder);
        }

        sortView(sortColumn, sortOrder);
    }
}

void CoinControlDialog::radioTreeMode(bool checked)
{
    if (checked && model)
        updateView();
}

void CoinControlDialog::radioListMode(bool checked)
{
    if (checked && model)
        updateView();
}

void CoinControlDialog::viewItemChanged(QTreeWidgetItem* item, int column)
{
    if (column == COLUMN_CHECKBOX && item->text(COLUMN_TXHASH).length() == 64)
    {
        COutPoint outpt(uint256(item->text(COLUMN_TXHASH).toStdString()), item->text(COLUMN_VOUT_INDEX).toUInt());

        if (item->checkState(COLUMN_CHECKBOX) == Qt::Unchecked)
            coinControl->UnSelect(outpt);
        else if (item->isDisabled())
            item->setCheckState(COLUMN_CHECKBOX, Qt::Unchecked);
        else
            coinControl->Select(outpt);

        if (ui->treeWidget->isEnabled())
            CoinControlDialog::updateLabels(model, this);
    }
}

QString CoinControlDialog::getPriorityLabel(double dPriority)
{
    if (dPriority > 576000ULL)
    {
        if      (dPriority > 5760000000ULL)   return tr("highest");
        else if (dPriority > 576000000ULL)    return tr("high");
        else if (dPriority > 57600000ULL)     return tr("medium-high");
        else                                    return tr("medium");
    }
    else
    {
        if      (dPriority > 5760ULL) return tr("low-medium");
        else if (dPriority > 58ULL)   return tr("low");
        else                            return tr("lowest");
    }
}

void CoinControlDialog::updateLabels(WalletModel *model, QDialog* dialog)
{
    if (!model) return;

    qint64 nPayAmount = 0;
    bool fLowOutput = false;
    bool fDust = false;
    CTransaction txDummy;
    Q_FOREACH(const qint64 &amount, CoinControlDialog::payAmounts)
    {
        nPayAmount += amount;

        if (amount > 0)
        {
            if (amount < CENT)
                fLowOutput = true;

            CTxOut txout(amount, (CScript)vector<unsigned char>(24, 0));
            txDummy.vout.push_back(txout);
        }
    }

    QString sPriorityLabel      = "";
    int64_t nAmount             = 0;
    int64_t nPayFee             = 0;
    int64_t nAfterFee           = 0;
    int64_t nChange             = 0;
    unsigned int nBytes         = 0;
    unsigned int nBytesInputs   = 0;
    double dPriority            = 0;
    double dPriorityInputs      = 0;
    unsigned int nQuantity      = 0;

    vector<COutPoint> vCoinControl;
    vector<COutput>   vOutputs;
    coinControl->ListSelected(vCoinControl);
    model->getOutputs(vCoinControl, vOutputs);

    BOOST_FOREACH(const COutput& out, vOutputs)
    {

        nQuantity++;

        nAmount += out.tx->vout[out.i].nValue;

        dPriorityInputs += (double)out.tx->vout[out.i].nValue * (out.nDepth+1);

        CTxDestination address;
        if(ExtractDestination(out.tx->vout[out.i].scriptPubKey, address))
        {
            CPubKey pubkey;
            CKeyID *keyid = boost::get< CKeyID >(&address);
            if (keyid && model->getPubKey(*keyid, pubkey))
                nBytesInputs += (pubkey.IsCompressed() ? 148 : 180);
            else
                nBytesInputs += 148;
        }
        else nBytesInputs += 148;
    }

    if (nQuantity > 0)
    {

        nBytes = nBytesInputs + ((CoinControlDialog::payAmounts.size() > 0 ? CoinControlDialog::payAmounts.size() + 1 : 2) * 34) + 10;

        dPriority = dPriorityInputs / nBytes;
        sPriorityLabel = CoinControlDialog::getPriorityLabel(dPriority);

        int64_t nFee = nTransactionFee * (1 + (int64_t)nBytes / 1000);

        int64_t nMinFee = txDummy.GetMinFee(1, GMF_SEND, nBytes);

        nPayFee = max(nFee, nMinFee);

		if(pwalletMain->fSplitBlock)
		{
			nPayFee = COIN * 1;
		}

        if (nPayAmount > 0)
        {
            nChange = nAmount - nPayFee - nPayAmount;

            if (nPayFee < CENT && nChange > 0 && nChange < CENT)
            {
                if (nChange < CENT)
                {
                    nPayFee = nChange;
                    nChange = 0;
                }
                else
                {
                    nChange = nChange + nPayFee - CENT;
                    nPayFee = CENT;
                }
            }

            if (nChange == 0)
                nBytes -= 34;
        }

        nAfterFee = nAmount - nPayFee;
        if (nAfterFee < 0)
            nAfterFee = 0;
    }

    int nDisplayUnit = BitcoinUnits::BTC;
    if (model && model->getOptionsModel())
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    QLabel *l1 = dialog->findChild<QLabel *>("labelCoinControlQuantity");
    QLabel *l2 = dialog->findChild<QLabel *>("labelCoinControlAmount");
    QLabel *l3 = dialog->findChild<QLabel *>("labelCoinControlFee");
    QLabel *l4 = dialog->findChild<QLabel *>("labelCoinControlAfterFee");
    QLabel *l5 = dialog->findChild<QLabel *>("labelCoinControlBytes");
    QLabel *l6 = dialog->findChild<QLabel *>("labelCoinControlPriority");
    QLabel *l7 = dialog->findChild<QLabel *>("labelCoinControlLowOutput");
    QLabel *l8 = dialog->findChild<QLabel *>("labelCoinControlChange");

    dialog->findChild<QLabel *>("labelCoinControlLowOutputText")->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlLowOutput")    ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlChangeText")   ->setEnabled(nPayAmount > 0);
    dialog->findChild<QLabel *>("labelCoinControlChange")       ->setEnabled(nPayAmount > 0);

    l1->setText(QString::number(nQuantity));
    l2->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAmount));
    l3->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nPayFee));
    l4->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nAfterFee));
    l5->setText(((nBytes > 0) ? "~" : "") + QString::number(nBytes));
    l6->setText(sPriorityLabel);
    l7->setText((fLowOutput ? (fDust ? tr("DUST") : tr("yes")) : tr("no")));
    l8->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, nChange));

    l5->setStyleSheet((nBytes >= 10000) ? "color:red;" : "");
    l6->setStyleSheet((dPriority <= 576000) ? "color:red;" : "");
    l7->setStyleSheet((fLowOutput) ? "color:red;" : "");
    l8->setStyleSheet((nChange > 0 && nChange < CENT) ? "color:red;" : "");

    l5->setToolTip(tr("This label turns red, if the transaction size is bigger than 10000 bytes.\n\n This means a fee of at least %1 per kb is required.\n\n Can vary +/- 1 Byte per input.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)));
    l6->setToolTip(tr("Transactions with higher priority get more likely into a block.\n\nThis label turns red, if the priority is smaller than \"medium\".\n\n This means a fee of at least %1 per kb is required.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)));
    l7->setToolTip(tr("This label turns red, if any recipient receives an amount smaller than %1.\n\n This means a fee of at least %2 is required. \n\n Amounts below 0.546 times the minimum relay fee are shown as DUST.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)).arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)));
    l8->setToolTip(tr("This label turns red, if the change is smaller than %1.\n\n This means a fee of at least %2 is required.").arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)).arg(BitcoinUnits::formatWithUnit(nDisplayUnit, CENT)));
    dialog->findChild<QLabel *>("labelCoinControlBytesText")    ->setToolTip(l5->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlPriorityText") ->setToolTip(l6->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlLowOutputText")->setToolTip(l7->toolTip());
    dialog->findChild<QLabel *>("labelCoinControlChangeText")   ->setToolTip(l8->toolTip());

    QLabel *label = dialog->findChild<QLabel *>("labelCoinControlInsuffFunds");
    if (label)
        label->setVisible(nChange < 0);
}

void CoinControlDialog::updateView()
{
    bool treeMode = ui->radioTreeMode->isChecked();

    ui->treeWidget->clear();
    ui->treeWidget->setEnabled(false);
    ui->treeWidget->setAlternatingRowColors(!treeMode);
    QFlags<Qt::ItemFlag> flgCheckbox=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    QFlags<Qt::ItemFlag> flgTristate=Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsTristate;

    int nDisplayUnit = BitcoinUnits::BTC;
    if (model && model->getOptionsModel())
        nDisplayUnit = model->getOptionsModel()->getDisplayUnit();

    map<QString, vector<COutput> > mapCoins;
    model->listCoins(mapCoins);

    BOOST_FOREACH(PAIRTYPE(QString, vector<COutput>) coins, mapCoins)
    {
        QTreeWidgetItem *itemWalletAddress = new QTreeWidgetItem();
        QString sWalletAddress = coins.first;
        QString sWalletLabel = "";
        if (model->getAddressTableModel())
            sWalletLabel = model->getAddressTableModel()->labelForAddress(sWalletAddress);
        if (sWalletLabel.length() == 0)
            sWalletLabel = tr("(no label)");

        if (treeMode)
        {

            ui->treeWidget->addTopLevelItem(itemWalletAddress);

            itemWalletAddress->setFlags(flgTristate);
            itemWalletAddress->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);

            for (int i = 0; i < ui->treeWidget->columnCount(); i++)
                itemWalletAddress->setBackground(i, QColor(248, 247, 246));

            itemWalletAddress->setText(COLUMN_LABEL, sWalletLabel);

            itemWalletAddress->setText(COLUMN_ADDRESS, sWalletAddress);
        }

        int64_t nSum = 0;
        double dPrioritySum = 0;
        int nChildren = 0;
        int nInputSum = 0;
		uint64_t nTxWeight = 0;
		uint64_t nDisplayWeight = 0;
		uint64_t nTxWeightSum = 0;
		GetLastBlockIndex(pindexBest, false);
		int64_t nBestHeight = pindexBest->nHeight;
		uint64_t nNetworkWeight = GetPoSKernelPS();

        BOOST_FOREACH(const COutput& out, coins.second)
        {

			int64_t nHeight = nBestHeight - out.nDepth;
			CBlockIndex* pindex = FindBlockByHeight(nHeight);

			int nInputSize = 148;
            nSum += out.tx->vout[out.i].nValue;
            nChildren++;

			model->getStakeWeightFromValue(out.tx->GetTxTime(), out.tx->vout[out.i].nValue, nTxWeight);
			if ((GetTime() - pindex->nTime) < (60*60*24*7))
				nDisplayWeight = 0;
			else
				nDisplayWeight = nTxWeight;

			nTxWeightSum += nDisplayWeight;

            QTreeWidgetItem *itemOutput;
            if (treeMode)    itemOutput = new QTreeWidgetItem(itemWalletAddress);
            else             itemOutput = new QTreeWidgetItem(ui->treeWidget);
            itemOutput->setFlags(flgCheckbox);
            itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Unchecked);

            CTxDestination outputAddress;
            QString sAddress = "";
            if(ExtractDestination(out.tx->vout[out.i].scriptPubKey, outputAddress))
            {
                sAddress = CBitcoinAddress(outputAddress).ToString().c_str();

                if (!treeMode || (!(sAddress == sWalletAddress)))
                    itemOutput->setText(COLUMN_ADDRESS, sAddress);

                CPubKey pubkey;
                CKeyID *keyid = boost::get< CKeyID >(&outputAddress);
                if (keyid && model->getPubKey(*keyid, pubkey) && !pubkey.IsCompressed())
                    nInputSize = 180;
            }

            if (!(sAddress == sWalletAddress))
            {

                itemOutput->setToolTip(COLUMN_LABEL, tr("change from %1 (%2)").arg(sWalletLabel).arg(sWalletAddress));
                itemOutput->setText(COLUMN_LABEL, tr("(change)"));
            }
            else if (!treeMode)
            {
                QString sLabel = "";
                if (model->getAddressTableModel())
                    sLabel = model->getAddressTableModel()->labelForAddress(sAddress);
                if (sLabel.length() == 0)
                    sLabel = tr("(no label)");
                itemOutput->setText(COLUMN_LABEL, sLabel);
            }

			uint64_t nBlockSize = out.tx->vout[out.i].nValue / 100000000;
            itemOutput->setText(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, out.tx->vout[out.i].nValue));
            itemOutput->setText(COLUMN_AMOUNT_INT64, strPad(QString::number(out.tx->vout[out.i].nValue), 15, " "));

			int64_t nTime = pindex->nTime;
            itemOutput->setText(COLUMN_DATE, QDateTime::fromTime_t(nTime).toString("yy-MM-dd hh:mm"));

            if (out.tx->IsCoinStake() && out.tx->GetBlocksToMaturity() > 0 && out.tx->GetDepthInMainChain() > 0) {
              itemOutput->setBackground(COLUMN_CONFIRMATIONS, Qt::red);
              itemOutput->setDisabled(true);
            }

            itemOutput->setText(COLUMN_CONFIRMATIONS, strPad(QString::number(out.nDepth), 8, " "));

            double dPriority = ((double)out.tx->vout[out.i].nValue  / (nInputSize + 78)) * (out.nDepth+1);
            itemOutput->setText(COLUMN_PRIORITY, CoinControlDialog::getPriorityLabel(dPriority));
            itemOutput->setText(COLUMN_PRIORITY_INT64, strPad(QString::number((int64_t)dPriority), 20, " "));
            dPrioritySum += (double)out.tx->vout[out.i].nValue  * (out.nDepth+1);
            nInputSum    += nInputSize;

			itemOutput->setText(COLUMN_WEIGHT, strPad(QString::number(nTxWeight), 8, " "));

			uint64_t nAge = (GetTime() - nTime);
			int64_t age = COIN * nAge / (1440 * 60);
			itemOutput->setText(COLUMN_AGE, strPad(BitcoinUnits::formatAge(nDisplayUnit, age), 2, " "));
			itemOutput->setText(COLUMN_AGE_INT64, strPad(QString::number(age), 15, " "));

			uint64_t nMin = 1;
			nBlockSize = qMax(nBlockSize, nMin);
			uint64_t nTimeToMaturity = 0;
			uint64_t nBlockWeight = qMax(nDisplayWeight, uint64_t(nBlockSize * 2));
			double dAge = nAge;
			if (172800 - dAge >= 0 )
				nTimeToMaturity = (172800 - nAge);
			else
				nTimeToMaturity = 0;
			uint64_t nAccuracyAdjustment = 1;
			uint64_t nEstimateTime = 60 * nNetworkWeight / nBlockWeight / nAccuracyAdjustment;
			uint64_t nMax = 999 * COIN;
			nEstimateTime = qMin((nEstimateTime + nTimeToMaturity) * COIN / (60*60*24), nMax);
			itemOutput->setText(COLUMN_TIMEESTIMATE, strPad(BitcoinUnits::formatAge(nDisplayUnit, nEstimateTime), 15, " "));

            uint256 txhash = out.tx->GetHash();
            itemOutput->setText(COLUMN_TXHASH, txhash.GetHex().c_str());

            itemOutput->setText(COLUMN_VOUT_INDEX, QString::number(out.i));

            if (coinControl->IsSelected(txhash, out.i))
                itemOutput->setCheckState(COLUMN_CHECKBOX,Qt::Checked);
        }

        if (treeMode)
        {
            dPrioritySum = dPrioritySum / (nInputSum + 78);
            itemWalletAddress->setText(COLUMN_CHECKBOX, "(" + QString::number(nChildren) + ")");
            itemWalletAddress->setText(COLUMN_AMOUNT, BitcoinUnits::format(nDisplayUnit, nSum));
            itemWalletAddress->setText(COLUMN_AMOUNT_INT64, strPad(QString::number(nSum), 15, " "));
            itemWalletAddress->setText(COLUMN_PRIORITY, CoinControlDialog::getPriorityLabel(dPrioritySum));
            itemWalletAddress->setText(COLUMN_PRIORITY_INT64, strPad(QString::number((int64_t)dPrioritySum), 20, " "));

			itemWalletAddress->setText(COLUMN_WEIGHT, strPad(QString::number((uint64_t)nTxWeightSum),8," "));
        }
    }

    if (treeMode)
    {
        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
            if (ui->treeWidget->topLevelItem(i)->checkState(COLUMN_CHECKBOX) == Qt::PartiallyChecked)
                ui->treeWidget->topLevelItem(i)->setExpanded(true);
    }

    sortView(sortColumn, sortOrder);

    ui->treeWidget->setEnabled(true);
}
