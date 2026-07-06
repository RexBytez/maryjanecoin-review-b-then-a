#ifndef SENDCOINSENTRY_H
#define SENDCOINSENTRY_H

#include <QFrame>

namespace Ui {
    class SendCoinsEntry;
}
class WalletModel;
class SendCoinsRecipient;

class SendCoinsEntry : public QFrame
{
    Q_OBJECT

public:
    explicit SendCoinsEntry(QWidget *parent = 0);
    ~SendCoinsEntry();

    void setModel(WalletModel *model);
    bool validate();
    SendCoinsRecipient getValue();

    bool isClear();

    void setValue(const SendCoinsRecipient &value);

    QWidget *setupTabChain(QWidget *prev);

    void setFocus();

public Q_SLOTS:
    void setRemoveEnabled(bool enabled);
    void clear();

Q_SIGNALS:
    void removeEntry(SendCoinsEntry *entry);
    void payAmountChanged();

private Q_SLOTS:
    void on_deleteButton_clicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();

private:
    Ui::SendCoinsEntry *ui;
    WalletModel *model;
};

#endif
