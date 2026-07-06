#ifndef TRANSACTIONVIEW_H
#define TRANSACTIONVIEW_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <QWidget>

class WalletModel;
class TransactionFilterProxy;

QT_BEGIN_NAMESPACE
class QTableView;
class QComboBox;
class QLineEdit;
class QModelIndex;
class QMenu;
class QFrame;
class QDateTimeEdit;
class QLabel;
QT_END_NAMESPACE

class TransactionView : public QWidget
{
    Q_OBJECT
public:
    explicit TransactionView(QWidget *parent = 0);
	QTableView *transactionView;

    void setModel(WalletModel *model);

    enum DateEnum
    {
        All,
        Today,
        ThisWeek,
        ThisMonth,
        LastMonth,
        ThisYear,
        Range
    };

private:
    WalletModel *model;
    TransactionFilterProxy *transactionProxyModel;

    QComboBox *dateWidget;
    QComboBox *typeWidget;
    QLineEdit *addressWidget;
    QLineEdit *amountWidget;
    QLabel *totalAmountWidgetLabel;
    QLabel *totalAmountWidget;

    QMenu *contextMenu;

    QFrame *dateRangeWidget;
    QDateTimeEdit *dateFrom;
    QDateTimeEdit *dateTo;

    QWidget *createDateRangeWidget();

private:
    void updateTotalAmount();
    void enableDateRangeWidget(bool enable);

private Q_SLOTS:
    void contextualMenu(const QPoint &);
    void dateRangeChanged();
    void showDetails();
    void copyAddress();
    void editLabel();
    void copyLabel();
    void copyAmount();
    void copyTxID();
	void showBrowser();
	void showQRCode();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);
	void blockBrowserSignal(QString transactionId);

public Q_SLOTS:
    void chooseDate(int idx);
    void chooseType(int idx);
    void changedPrefix(const QString &prefix);
    void changedAmount(const QString &amount);
    void exportClicked();
    void focusTransaction(const QModelIndex&);

};

#endif
