#ifndef QVALUECOMBOBOX_H
#define QVALUECOMBOBOX_H

#include <QComboBox>
#include <QVariant>

class QValueComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)
public:
    explicit QValueComboBox(QWidget *parent = 0);

    QVariant value() const;
    void setValue(const QVariant &value);

    void setRole(int role);

Q_SIGNALS:
    void valueChanged();

public Q_SLOTS:

private:
    int role;

private Q_SLOTS:
    void handleSelectionChanged(int idx);
};

#endif
