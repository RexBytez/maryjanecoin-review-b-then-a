#ifndef QCOMBOBOCFILTERCOINS_H
#define QCOMBOBOCFILTERCOINS_H

#include <QtGui>
#include <QMessageBox>
#include <QComboBox>
#include <QVariant>

class QComboBoxFilterCoins: public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)

public:
    explicit QComboBoxFilterCoins(QWidget *parent = 0);

    QVariant value() const;
    void setValue(const QVariant &value);

    void setRole(int role);

Q_SIGNALS:
    void valueChanged();

private:
    int role;

private Q_SLOTS:
    void handleSelectionChanged(int idx);
};

#endif
