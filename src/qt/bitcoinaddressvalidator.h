#ifndef BITCOINADDRESSVALIDATOR_H
#define BITCOINADDRESSVALIDATOR_H

#include <QRegExpValidator>

class BitcoinAddressValidator : public QValidator
{
    Q_OBJECT
public:
    explicit BitcoinAddressValidator(QObject *parent = 0);

    State validate(QString &input, int &pos) const;

    static const int MaxAddressLength = 35;
Q_SIGNALS:

public Q_SLOTS:

};

#endif
