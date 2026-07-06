#include "charitydialog.h"
#include <qt/forms/ui_charitydialog.h>

#include "walletmodel.h"
#include "base58.h"
#include "addressbookpage.h"
#include "init.h"
#include "walletdb.h"
#include "util.h"

#include <QMessageBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QCheckBox>
#include <QHeaderView>
#include <QIntValidator>
#include <QTimer>
#include <QShowEvent>
#include <QPaintEvent>
#include <QtWidgets>

#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

StakeForCharityDialog::StakeForCharityDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StakeForCharityDialog),
    model(0),
    fWidgetsCreated(false),
    fCreatingWidgets(false)
{
    ui->setupUi(this);

    if (ui->activateCheckBox && ui->saveButton && ui->clearButton)
    {
        connect(ui->activateCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_activateCheckBox_toggled(bool)));
        connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(on_saveButton_clicked()));
        connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(on_clearButton_clicked()));
    }

}

StakeForCharityDialog::~StakeForCharityDialog()
{
    delete ui;
}

void StakeForCharityDialog::setModel(WalletModel *model)
{

    this->model = model;

}

void StakeForCharityDialog::showEvent(QShowEvent *event)
{

    QWidget::showEvent(event);
}

void StakeForCharityDialog::paintEvent(QPaintEvent *event)
{
    if (!ui || !ui->charityTable)
    {
        return;
    }

    QWidget::paintEvent(event);
}

void StakeForCharityDialog::ensureWidgetsCreated()
{

    if (!ui || !ui->charityTable)
    {
        return;
    }

    if (ui->charityTable->rowCount() > 0 && ui->charityTable->columnCount() > 0)
    {

        QWidget *existingWidget = ui->charityTable->cellWidget(0, 0);
        if (existingWidget)
        {
            return;
        }
    }

    ui->charityTable->setUpdatesEnabled(false);
    ui->charityTable->blockSignals(true);

    ui->charityTable->setRowCount(5);
    ui->charityTable->setColumnCount(4);

    ui->charityTable->setColumnWidth(0, 80);
    ui->charityTable->setColumnWidth(1, 350);
    ui->charityTable->setColumnWidth(2, 100);
    ui->charityTable->setColumnWidth(3, 80);

    QStringList headers;
    headers << "Enabled" << "Address" << "Percentage (%)" << "Action";
    ui->charityTable->setHorizontalHeaderLabels(headers);

    QHeaderView *horizontalHeader = ui->charityTable->horizontalHeader();
    QHeaderView *verticalHeader = ui->charityTable->verticalHeader();
    if (horizontalHeader)
    {
        horizontalHeader->setDefaultSectionSize(100);
        horizontalHeader->setStretchLastSection(true);
    }
    if (verticalHeader)
    {
        verticalHeader->setVisible(false);
    }

    ui->charityTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    fCreatingWidgets = true;

    for (int i = 0; i < 5; i++)
    {
        addRow(i, QString(), 0, false);
    }

    ui->charityTable->setUpdatesEnabled(true);
    ui->charityTable->blockSignals(false);

    fCreatingWidgets = false;
}

void StakeForCharityDialog::setupTable()
{

}

void StakeForCharityDialog::addRow(int row, const QString& address, int percent, bool enabled)
{
    if (!ui || !ui->charityTable)
    {
        return;
    }

    if (row < 0 || row >= 5)
    {
        return;
    }

    QCheckBox *enableCheckbox = new QCheckBox(ui->charityTable);
    enableCheckbox->blockSignals(true);
    enableCheckbox->setChecked(enabled);
    enableCheckbox->setProperty("row", row);

    connect(enableCheckbox, SIGNAL(toggled(bool)), this, SLOT(handleEnableCheckboxToggled()));
    ui->charityTable->setCellWidget(row, 0, enableCheckbox);
    enableCheckbox->blockSignals(false);

    QWidget *addressWidget = new QWidget(ui->charityTable);
    QHBoxLayout *addressLayout = new QHBoxLayout(addressWidget);
    addressLayout->setContentsMargins(2, 2, 2, 2);
    addressLayout->setSpacing(2);

    QLineEdit *addressEdit = new QLineEdit(addressWidget);
    addressEdit->blockSignals(true);
    addressEdit->setText(address);
    addressEdit->setPlaceholderText("Enter MaryJaneCoin address");
    addressEdit->setProperty("row", row);
    connect(addressEdit, SIGNAL(textChanged(QString)), this, SLOT(on_addressTextChanged()));
    addressLayout->addWidget(addressEdit);
    addressEdit->blockSignals(false);

    QPushButton *addressBookButton = new QPushButton(addressWidget);
    addressBookButton->setIcon(QIcon(":/icons/address-book"));
    addressBookButton->setProperty("row", row);
    addressBookButton->setToolTip(tr("Choose an address from the address book"));
    addressBookButton->setMaximumWidth(30);
    addressBookButton->setMaximumHeight(25);

    connect(addressBookButton, SIGNAL(clicked()), this, SLOT(handleAddressBookButtonClicked()));
    addressLayout->addWidget(addressBookButton);

    ui->charityTable->setCellWidget(row, 1, addressWidget);

    QLineEdit *percentEdit = new QLineEdit(ui->charityTable);
    percentEdit->blockSignals(true);
    percentEdit->setText(percent > 0 ? QString::number(percent) : QString());
    percentEdit->setPlaceholderText("1-100");
    percentEdit->setValidator(new QIntValidator(1, 100, this));
    percentEdit->setProperty("row", row);
    percentEdit->setMaximumWidth(80);

    connect(percentEdit, SIGNAL(textChanged(QString)), this, SLOT(on_percentTextChanged()));
    ui->charityTable->setCellWidget(row, 2, percentEdit);
    percentEdit->blockSignals(false);

    QPushButton *deleteButton = new QPushButton("Remove", ui->charityTable);
    deleteButton->setProperty("row", row);
    deleteButton->setMaximumWidth(70);

    connect(deleteButton, SIGNAL(clicked()), this, SLOT(handleDeleteRowButtonClicked()));
    ui->charityTable->setCellWidget(row, 3, deleteButton);

    QCheckBox *checkbox = qobject_cast<QCheckBox*>(ui->charityTable->cellWidget(row, 0));
    QWidget *addrWidget = qobject_cast<QWidget*>(ui->charityTable->cellWidget(row, 1));
    QLineEdit *pctEdit = qobject_cast<QLineEdit*>(ui->charityTable->cellWidget(row, 2));
    QPushButton *delButton = qobject_cast<QPushButton*>(ui->charityTable->cellWidget(row, 3));

    if (address.isEmpty() && percent == 0)
    {

        if (checkbox)
            checkbox->setEnabled(false);
        if (delButton)
            delButton->setEnabled(false);

        if (addrWidget)
        {

            QLayout *layout = addrWidget->layout();
            if (layout && layout->count() > 0)
            {
                QWidget *widget = layout->itemAt(0)->widget();
                QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
                if (edit)
                {
                    edit->setStyleSheet("color: gray;");
                    edit->setEnabled(false);
                }
            }
        }

        if (pctEdit)
        {
            pctEdit->setStyleSheet("color: gray;");
            pctEdit->setEnabled(false);
        }
    }
    else
    {

        if (checkbox)
            checkbox->setEnabled(true);
        if (delButton)
            delButton->setEnabled(true);

        if (addrWidget)
        {

            QLayout *layout = addrWidget->layout();
            if (layout && layout->count() > 0)
            {
                QWidget *widget = layout->itemAt(0)->widget();
                QLineEdit *edit = qobject_cast<QLineEdit*>(widget);
                if (edit)
                    edit->setEnabled(true);
            }
        }

        if (pctEdit)
            pctEdit->setEnabled(true);
    }

}

void StakeForCharityDialog::refreshTable()
{

    if (!ui || !ui->charityTable)
    {
        return;
    }

    if (!fWidgetsCreated)
    {
        ensureWidgetsCreated();
        fWidgetsCreated = true;
    }

    for (int i = 0; i < 5; i++)
    {
        QString address;
        int percent = 0;
        bool enabled = false;

        if (pwalletMain)
        {
            try {
                if (i < (int)pwalletMain->vStakeForCharity.size())
                {
                    address = QString::fromStdString(pwalletMain->vStakeForCharity[i].strAddress);
                    percent = pwalletMain->vStakeForCharity[i].nPercent;
                    enabled = true;
                }
            } catch (const std::exception& e) {

            } catch (...) {

            }
        }

        addRow(i, address, percent, enabled);
    }

    if (pwalletMain && ui->activateCheckBox)
    {
        try {
            bool fActive = pwalletMain->fStakeForCharity;
            ui->activateCheckBox->setChecked(fActive);
        } catch (const std::exception& e) {

        } catch (...) {

        }
    }

    updateTotalPercent();
    updateStatus();
}

void StakeForCharityDialog::updateStatus()
{
    if (!ui || !ui->statusLabel || !ui->activateCheckBox)
    {
        return;
    }

    if (!pwalletMain)
    {
        ui->statusLabel->setText("Status: Not Active");
        ui->statusLabel->setStyleSheet("color: red; font-weight: bold;");
        ui->activateCheckBox->setChecked(false);
        ui->activateCheckBox->setEnabled(false);
        return;
    }

    ui->activateCheckBox->setEnabled(true);

    bool isActive = false;
    try {
        isActive = pwalletMain->fStakeForCharity;
    } catch (const std::exception& e) {
        isActive = false;
    } catch (...) {
        isActive = false;
    }

    if (isActive)
    {
        ui->statusLabel->setText("Status: Active");
        ui->statusLabel->setStyleSheet("color: green; font-weight: bold;");
    }
    else
    {
        ui->statusLabel->setText("Status: Not Active");
        ui->statusLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}

void StakeForCharityDialog::updateTotalPercent()
{
    if (!ui || !ui->charityTable || !ui->totalPercentLabel)
    {
        return;
    }

    int total = 0;

    for (int row = 0; row < 5; row++)
    {
        QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);
        QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);

        if (!checkboxWidget || !percentWidget)
            continue;

        QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);
        QLineEdit *percentEdit = qobject_cast<QLineEdit*>(percentWidget);

        if (checkbox && checkbox->isChecked() && percentEdit && !percentEdit->text().isEmpty())
        {
            bool ok;
            int percent = percentEdit->text().toInt(&ok);
            if (ok && percent > 0)
            {
                total += percent;
            }
        }
    }

    QString totalText = QString("Total: %1%").arg(total);
    if (total > 100)
    {
        totalText += " (Exceeds 100%!)";
        ui->totalPercentLabel->setStyleSheet("color: red; font-weight: bold;");
    }
    else if (total == 100)
    {
        ui->totalPercentLabel->setStyleSheet("color: green; font-weight: bold;");
    }
    else
    {
        ui->totalPercentLabel->setStyleSheet("color: black; font-weight: bold;");
    }

    ui->totalPercentLabel->setText(totalText);
}

bool StakeForCharityDialog::validateAddress(const QString& address)
{
    if (address.isEmpty())
        return true;

    std::string strAddress = address.toStdString();
    return CBitcoinAddress(strAddress).IsValid();
}

bool StakeForCharityDialog::validatePercent(int percent, int excludeRow)
{
    if (percent < 1 || percent > 100)
        return false;

    int total = 0;
    for (int row = 0; row < 5; row++)
    {
        if (row == excludeRow)
            continue;

        QCheckBox *checkbox = qobject_cast<QCheckBox*>(ui->charityTable->cellWidget(row, 0));
        QLineEdit *percentEdit = qobject_cast<QLineEdit*>(ui->charityTable->cellWidget(row, 2));

        if (checkbox && checkbox->isChecked() && percentEdit && !percentEdit->text().isEmpty())
        {
            bool ok;
            int rowPercent = percentEdit->text().toInt(&ok);
            if (ok && rowPercent > 0)
            {
                total += rowPercent;
            }
        }
    }

    return (total + percent) <= 100;
}

QLineEdit* StakeForCharityDialog::getAddressEdit(int row)
{
    if (!ui || !ui->charityTable)
        return NULL;

    QWidget *addressWidget = qobject_cast<QWidget*>(ui->charityTable->cellWidget(row, 1));
    if (!addressWidget)
        return NULL;

    QLayout *layout = addressWidget->layout();
    if (!layout || layout->count() == 0)
        return NULL;

    QLayoutItem *item = layout->itemAt(0);
    if (!item)
        return NULL;

    QWidget *widget = item->widget();
    return qobject_cast<QLineEdit*>(widget);
}

void StakeForCharityDialog::on_addressTextChanged()
{

    if (fCreatingWidgets || !fWidgetsCreated)
        return;

    if (!ui || !ui->charityTable)
        return;

    QLineEdit *addressEdit = qobject_cast<QLineEdit*>(sender());
    if (!addressEdit)
        return;

    int row = addressEdit->property("row").toInt();
    if (row < 0 || row >= 5)
        return;

    QString text = addressEdit->text();

    QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);
    QWidget *deleteWidget = ui->charityTable->cellWidget(row, 3);
    QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);

    QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);
    QPushButton *deleteButton = qobject_cast<QPushButton*>(deleteWidget);
    QLineEdit *percentEdit = qobject_cast<QLineEdit*>(percentWidget);

    if (!text.isEmpty())
    {
        if (checkbox)
        {
            checkbox->setEnabled(true);
            if (!checkbox->isChecked())
                checkbox->setChecked(true);
        }
        if (deleteButton)
            deleteButton->setEnabled(true);
        if (percentEdit)
            percentEdit->setEnabled(true);
        addressEdit->setStyleSheet("");
    }
    else
    {

        if (percentEdit && percentEdit->text().isEmpty())
        {
            if (checkbox)
            {
                checkbox->setChecked(false);
                checkbox->setEnabled(false);
            }
            if (deleteButton)
                deleteButton->setEnabled(false);
            if (percentEdit)
                percentEdit->setEnabled(false);
            addressEdit->setStyleSheet("color: gray;");
        }
    }
}

void StakeForCharityDialog::handleAddressBookButtonClicked()
{

    if (fCreatingWidgets || !fWidgetsCreated)
        return;

    if (!ui || !ui->charityTable)
        return;

    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button)
        return;

    int row = button->property("row").toInt();
    if (row < 0 || row >= 5)
        return;

    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::SendingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
        {
            QLineEdit *addressEdit = getAddressEdit(row);
            if (addressEdit)
            {
                addressEdit->setText(dlg.getReturnValue());
                addressEdit->setFocus();
            }
        }
    }
}

void StakeForCharityDialog::on_percentTextChanged()
{

    if (fCreatingWidgets || !fWidgetsCreated)
        return;

    if (!ui || !ui->charityTable)
        return;

    QLineEdit *percentEdit = qobject_cast<QLineEdit*>(sender());
    if (!percentEdit)
        return;

    int row = percentEdit->property("row").toInt();
    if (row < 0 || row >= 5)
        return;

    QString text = percentEdit->text();

    QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);
    QWidget *deleteWidget = ui->charityTable->cellWidget(row, 3);

    QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);
    QPushButton *deleteButton = qobject_cast<QPushButton*>(deleteWidget);
    QLineEdit *addressEdit = getAddressEdit(row);

    if (!text.isEmpty())
    {
        if (checkbox)
        {
            checkbox->setEnabled(true);
            if (!checkbox->isChecked())
                checkbox->setChecked(true);
        }
        if (deleteButton)
            deleteButton->setEnabled(true);
        if (addressEdit)
            addressEdit->setEnabled(true);
        percentEdit->setStyleSheet("");
    }
    else
    {

        if (addressEdit && addressEdit->text().isEmpty())
        {
            if (checkbox)
            {
                checkbox->setChecked(false);
                checkbox->setEnabled(false);
            }
            if (deleteButton)
                deleteButton->setEnabled(false);
            if (addressEdit)
                addressEdit->setEnabled(false);
            percentEdit->setStyleSheet("color: gray;");
        }
    }

    updateTotalPercent();
}

void StakeForCharityDialog::handleEnableCheckboxToggled()
{

    if (fCreatingWidgets || !fWidgetsCreated)
        return;

    if (!ui || !ui->charityTable)
        return;

    QCheckBox *checkbox = qobject_cast<QCheckBox*>(sender());
    if (!checkbox)
        return;

    int row = checkbox->property("row").toInt();
    if (row < 0 || row >= 5)
        return;

    QLineEdit *addressEdit = getAddressEdit(row);
    QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);
    QWidget *deleteWidget = ui->charityTable->cellWidget(row, 3);

    QLineEdit *percentEdit = qobject_cast<QLineEdit*>(percentWidget);
    QPushButton *deleteButton = qobject_cast<QPushButton*>(deleteWidget);

    bool enabled = checkbox->isChecked();
    bool hasData = addressEdit && !addressEdit->text().isEmpty();

    if (addressEdit)
    {
        addressEdit->setEnabled(enabled || hasData);
        if (enabled && addressEdit->text().isEmpty())
            addressEdit->setStyleSheet("");
        else if (!enabled && addressEdit->text().isEmpty())
            addressEdit->setStyleSheet("color: gray;");
    }

    if (percentEdit)
    {
        percentEdit->setEnabled(enabled || hasData);
        if (enabled && percentEdit->text().isEmpty())
            percentEdit->setStyleSheet("");
        else if (!enabled && percentEdit->text().isEmpty())
            percentEdit->setStyleSheet("color: gray;");
    }

    if (deleteButton)
        deleteButton->setEnabled(hasData);

    updateTotalPercent();
}

void StakeForCharityDialog::handleDeleteRowButtonClicked()
{

    if (fCreatingWidgets || !fWidgetsCreated)
        return;

    if (!ui || !ui->charityTable)
        return;

    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button)
        return;

    int row = button->property("row").toInt();
    if (row < 0 || row >= 5)
        return;

    QLineEdit *addressEdit = getAddressEdit(row);
    QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);
    QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);

    QLineEdit *percentEdit = qobject_cast<QLineEdit*>(percentWidget);
    QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);

    if (addressEdit)
        addressEdit->clear();
    if (percentEdit)
        percentEdit->clear();
    if (checkbox)
    {
        checkbox->setChecked(false);
        checkbox->setEnabled(false);
    }
    button->setEnabled(false);

    if (addressEdit)
        addressEdit->setStyleSheet("color: gray;");
    if (percentEdit)
        percentEdit->setStyleSheet("color: gray;");

    updateTotalPercent();
}

void StakeForCharityDialog::on_activateCheckBox_toggled(bool checked)
{
    if (!ui || !ui->activateCheckBox || !ui->messageLabel)
        return;

    if (!pwalletMain)
    {
        ui->activateCheckBox->setChecked(false);
        ui->messageLabel->setText(tr("Wallet not loaded. Please unlock your wallet first."));
        ui->messageLabel->setStyleSheet("color: red;");
        return;
    }

    if (!ui->charityTable)
        return;

    if (checked)
    {

        bool hasValidEntry = false;
        QString firstAddress;

        for (int row = 0; row < 5; row++)
        {
            QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);
            QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);

            if (!checkboxWidget || !percentWidget)
                continue;

            QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);
            QLineEdit *addressEdit = getAddressEdit(row);

            if (checkbox && checkbox->isChecked() && addressEdit && !addressEdit->text().isEmpty())
            {
                if (validateAddress(addressEdit->text()))
                {
                    hasValidEntry = true;
                    if (firstAddress.isEmpty())
                        firstAddress = addressEdit->text();
                }
            }
        }

        if (!hasValidEntry)
        {
            ui->activateCheckBox->setChecked(false);
            ui->messageLabel->setText(tr("Cannot activate: Please configure at least one valid charity address first."));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }

        int total = 0;
        for (int row = 0; row < 5; row++)
        {
            QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);
            QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);

            if (!checkboxWidget || !percentWidget)
                continue;

            QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);
            QLineEdit *percentEdit = qobject_cast<QLineEdit*>(percentWidget);

            if (checkbox && checkbox->isChecked() && percentEdit && !percentEdit->text().isEmpty())
            {
                bool ok;
                int percent = percentEdit->text().toInt(&ok);
                if (ok && percent > 0)
                    total += percent;
            }
        }

        if (total > 100)
        {
            ui->activateCheckBox->setChecked(false);
            ui->messageLabel->setText(tr("Cannot activate: Total percentage exceeds 100% (Current: %1%).").arg(total));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }

        if (total == 0)
        {
            ui->activateCheckBox->setChecked(false);
            ui->messageLabel->setText(tr("Cannot activate: Please configure at least one charity address with a percentage."));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }
    }

    pwalletMain->fStakeForCharity = checked;

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (pwalletMain->fFileBacked)
    {
        if (!walletdb.WriteCharitySettings(checked))
        {
            ui->messageLabel->setText(tr("Failed to save activation status to database."));
            ui->messageLabel->setStyleSheet("color: red;");
            pwalletMain->fStakeForCharity = !checked;
            ui->activateCheckBox->setChecked(!checked);
            return;
        }
    }

    updateStatus();

    if (checked)
    {
        ui->messageLabel->setText(tr("Stake For Charity activated successfully."));
        ui->messageLabel->setStyleSheet("color: green;");
    }
    else
    {
        ui->messageLabel->setText(tr("Stake For Charity deactivated."));
        ui->messageLabel->setStyleSheet("color: orange;");
    }
}

void StakeForCharityDialog::saveToDatabase()
{
    if (!ui || !ui->charityTable || !ui->messageLabel)
        return;

    if (!pwalletMain)
    {
        ui->messageLabel->setText(tr("Wallet not loaded. Please unlock your wallet first."));
        ui->messageLabel->setStyleSheet("color: red;");
        return;
    }

    std::vector<StakeForCharityEntry> vNewCharity;
    std::vector<std::string> seenAddresses;

    for (int row = 0; row < 5; row++)
    {
        QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);
        QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);

        if (!checkboxWidget || !percentWidget)
            continue;

        QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);
        QLineEdit *addressEdit = getAddressEdit(row);
        QLineEdit *percentEdit = qobject_cast<QLineEdit*>(percentWidget);

        if (!checkbox || !addressEdit || !percentEdit)
            continue;

        if (!checkbox->isChecked())
            continue;

        QString addressStr = addressEdit->text().trimmed();
        QString percentStr = percentEdit->text().trimmed();

        if (addressStr.isEmpty() || percentStr.isEmpty())
            continue;

        if (!validateAddress(addressStr))
        {
            ui->messageLabel->setText(tr("Invalid address in row %1: %2").arg(row + 1).arg(addressStr));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }

        std::string strAddress = addressStr.toStdString();
        bool isDuplicate = false;
        for (unsigned int i = 0; i < seenAddresses.size(); i++)
        {
            if (seenAddresses[i] == strAddress)
            {
                isDuplicate = true;
                break;
            }
        }

        if (isDuplicate)
        {
            ui->messageLabel->setText(tr("Duplicate address in row %1: %2").arg(row + 1).arg(addressStr));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }

        bool ok;
        int percent = percentStr.toInt(&ok);
        if (!ok || percent < 1 || percent > 100)
        {
            ui->messageLabel->setText(tr("Invalid percentage in row %1: %2 (must be 1-100)").arg(row + 1).arg(percentStr));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }

        int currentTotal = 0;
        for (unsigned int i = 0; i < vNewCharity.size(); i++)
            currentTotal += vNewCharity[i].nPercent;

        if (currentTotal + percent > 100)
        {
            ui->messageLabel->setText(tr("Total percentage would exceed 100%. Current: %1%, Adding: %2%").arg(currentTotal).arg(percent));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }

        seenAddresses.push_back(strAddress);
        StakeForCharityEntry entry(strAddress, percent);
        vNewCharity.push_back(entry);
    }

    int total = 0;
    for (unsigned int i = 0; i < vNewCharity.size(); i++)
        total += vNewCharity[i].nPercent;

    if (total > 100)
    {
        ui->messageLabel->setText(tr("Total percentage exceeds 100% (Current: %1%).").arg(total));
        ui->messageLabel->setStyleSheet("color: red;");
        return;
    }

    std::vector<StakeForCharityEntry> vOldCharity = pwalletMain->vStakeForCharity;
    pwalletMain->vStakeForCharity = vNewCharity;

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (pwalletMain->fFileBacked)
    {
        if (!walletdb.EraseCharity(vOldCharity))
        {
            pwalletMain->vStakeForCharity = vOldCharity;
            ui->messageLabel->setText(tr("Failed to update database (erase failed)."));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }

        if (!walletdb.WriteCharity(pwalletMain->vStakeForCharity))
        {
            pwalletMain->vStakeForCharity = vOldCharity;
            ui->messageLabel->setText(tr("Failed to save to database."));
            ui->messageLabel->setStyleSheet("color: red;");
            return;
        }
    }

    ui->messageLabel->setText(tr("Charity configuration saved successfully. %1 address(es) configured, Total: %2%.").arg(vNewCharity.size()).arg(total));
    ui->messageLabel->setStyleSheet("color: green;");

    updateTotalPercent();
    refreshTable();
}

void StakeForCharityDialog::on_saveButton_clicked()
{
    saveToDatabase();
}

void StakeForCharityDialog::on_clearButton_clicked()
{
    if (!ui || !ui->charityTable || !ui->messageLabel)
        return;

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Clear All Addresses"),
        tr("Are you sure you want to clear all charity addresses?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {

        for (int row = 0; row < 5; row++)
        {
            QLineEdit *addressEdit = getAddressEdit(row);
            QWidget *percentWidget = ui->charityTable->cellWidget(row, 2);
            QWidget *checkboxWidget = ui->charityTable->cellWidget(row, 0);
            QWidget *deleteWidget = ui->charityTable->cellWidget(row, 3);

            QLineEdit *percentEdit = qobject_cast<QLineEdit*>(percentWidget);
            QCheckBox *checkbox = qobject_cast<QCheckBox*>(checkboxWidget);
            QPushButton *deleteButton = qobject_cast<QPushButton*>(deleteWidget);

            if (addressEdit)
                addressEdit->clear();
            if (percentEdit)
                percentEdit->clear();
            if (checkbox)
            {
                checkbox->setChecked(false);
                checkbox->setEnabled(false);
            }
            if (deleteButton)
                deleteButton->setEnabled(false);

            if (addressEdit)
                addressEdit->setStyleSheet("color: gray;");
            if (percentEdit)
                percentEdit->setStyleSheet("color: gray;");
        }

        if (pwalletMain && ui->activateCheckBox)
        {
            std::vector<StakeForCharityEntry> vOldCharity = pwalletMain->vStakeForCharity;
            pwalletMain->vStakeForCharity.clear();
            pwalletMain->fStakeForCharity = false;
            ui->activateCheckBox->setChecked(false);

            CWalletDB walletdb(pwalletMain->strWalletFile);
            if (pwalletMain->fFileBacked)
            {
                walletdb.EraseCharity(vOldCharity);
                walletdb.WriteCharitySettings(false);
            }
        }
        else if (ui->activateCheckBox)
        {
            ui->activateCheckBox->setChecked(false);
        }

        updateTotalPercent();
        updateStatus();
        ui->messageLabel->setText(tr("All charity addresses cleared."));
        ui->messageLabel->setStyleSheet("color: orange;");
    }
}
