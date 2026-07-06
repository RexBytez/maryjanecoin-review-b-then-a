#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QtGui>
#include <QtNetwork>
#include "clientmodel.h"
#include "serveur.h"

namespace Ui
{
    class ChatWindowClass;
}

class ChatWindow : public QWidget
{
    Q_OBJECT

public:
    ChatWindow(QWidget *parent = 0);
    ~ChatWindow();
    void setModel(ClientModel *model);
    Serveur * currentTab();
	Q_SIGNALS:
		void changeTab();

	public Q_SLOTS:
		void sendCommande();
        void connecte();
		void closeTab();

		void tabChanged(int index);

		void tabJoined();
		void tabJoining();
        void disconnectFromServer();
        void tabClosing(int index);

private:
	Ui::ChatWindowClass *ui;
    ClientModel *model;
    QMap<QString,Serveur *> serveurs;
	bool joining;
	void closeEvent(QCloseEvent *event);

};

#endif
