#ifndef ASKPASSPHRASEDIALOG_H
#define ASKPASSPHRASEDIALOG_H

#include <QDialog>

namespace Ui {
    class AskPassphraseDialog;
}

class WalletModel;

class AskPassphraseDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        Encrypt,
        Unlock,
		UnlockForMint,
        ChangePass,
        Decrypt
    };

    explicit AskPassphraseDialog(Mode mode, QWidget *parent = 0);
    ~AskPassphraseDialog();

    void accept();

    void setModel(WalletModel *model);

private:
    Ui::AskPassphraseDialog *ui;
    Mode mode;
    WalletModel *model;
    bool fCapsLock;

private Q_SLOTS:
    void textChanged();
    bool event(QEvent *event);
    bool eventFilter(QObject *, QEvent *event);
};

#endif
