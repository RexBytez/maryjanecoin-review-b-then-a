#ifndef ADDRESSBOOKPAGE_H
#define ADDRESSBOOKPAGE_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <QDialog>

namespace Ui {
    class AddressBookPage;
}
class AddressTableModel;
class OptionsModel;

QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
QT_END_NAMESPACE

class AddressBookPage : public QDialog
{
    Q_OBJECT

public:
    enum Tabs {
        SendingTab = 0,
        ReceivingTab = 1
    };

    enum Mode {
        ForSending,
        ForEditing
    };

    explicit AddressBookPage(Mode mode, Tabs tab, QWidget *parent = 0);
    ~AddressBookPage();

    void setModel(AddressTableModel *model);
    void setOptionsModel(OptionsModel *optionsModel);
    const QString &getReturnValue() const { return returnValue; }

public Q_SLOTS:
    void done(int retval);
    void exportClicked();

private:
    Ui::AddressBookPage *ui;
    AddressTableModel *model;
    OptionsModel *optionsModel;
    Mode mode;
    Tabs tab;
    QString returnValue;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *deleteAction;
    QString newAddressToSelect;

private Q_SLOTS:
	void on_exportButton_clicked();
    void on_deleteButton_clicked();
    void on_newAddressButton_clicked();
	void on_deleteCheckBox_clicked();

    void on_copyToClipboard_clicked();
    void on_signMessage_clicked();
    void on_verifyMessage_clicked();
    void selectionChanged();
    void on_showQRCode_clicked();

    void contextualMenu(const QPoint &point);

    void onCopyLabelAction();

    void onEditAction();

    void selectNewAddress(const QModelIndex &parent, int begin, int end);

Q_SIGNALS:
    void signMessage(QString addr);
    void verifyMessage(QString addr);
};

#endif
