#ifndef BITCOINFIELD_H
#define BITCOINFIELD_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QDoubleSpinBox;
class QValueComboBox;
QT_END_NAMESPACE

class BitcoinAmountField: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qint64 value READ value WRITE setValue NOTIFY textChanged USER true)
public:
    explicit BitcoinAmountField(QWidget *parent = 0);

    qint64 value(bool *valid=0) const;
    void setValue(qint64 value);

    void setValid(bool valid);

    bool validate();

    void setDisplayUnit(int unit);

    void clear();

    QWidget *setupTabChain(QWidget *prev);

Q_SIGNALS:
    void textChanged();

protected:

    bool eventFilter(QObject *object, QEvent *event);

private:
    QDoubleSpinBox *amount;
    QValueComboBox *unit;
    int currentUnit;

    void setText(const QString &text);
    QString text() const;

private Q_SLOTS:
    void unitChanged(int idx);

};

#endif
