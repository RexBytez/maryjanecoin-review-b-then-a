#ifndef CHARITYDIALOG_H
#define CHARITYDIALOG_H

#include <QWidget>
#include <QTableWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

namespace Ui {
class StakeForCharityDialog;
}
class WalletModel;
class StakeForCharityDialog : public QWidget
{
    Q_OBJECT

public:
    explicit StakeForCharityDialog(QWidget *parent = 0);
    ~StakeForCharityDialog();

    void setModel(WalletModel *model);

public Q_SLOTS:
    void on_activateCheckBox_toggled(bool checked);
    void on_saveButton_clicked();
    void on_clearButton_clicked();
    void refreshTable();
    void updateStatus();

private Q_SLOTS:

    void handleEnableCheckboxToggled();
    void handleDeleteRowButtonClicked();
    void handleAddressBookButtonClicked();
    void on_addressTextChanged();
    void on_percentTextChanged();

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::StakeForCharityDialog *ui;
    WalletModel *model;
    bool fWidgetsCreated;
    bool fCreatingWidgets;
    void setupTable();
    void addRow(int row, const QString& address = QString(), int percent = 0, bool enabled = true);
    QLineEdit* getAddressEdit(int row);
    void updateTotalPercent();
    bool validateAddress(const QString& address);
    bool validatePercent(int percent, int row = -1);
    void saveToDatabase();
    void ensureWidgetsCreated();
};

#endif
