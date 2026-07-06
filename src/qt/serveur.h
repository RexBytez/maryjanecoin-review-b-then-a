#ifndef SERVEUR_H
#define SERVEUR_H

#include <QWidget>
#include <QListView>
#include <QTextEdit>
#include <QtGui>
#include <QtNetwork>
#include <QSystemTrayIcon>

class Serveur : public QTcpSocket
{
	Q_OBJECT

	public:
        Serveur();
		QTextEdit *affichage;
        QListView *userList;
		QString pseudo,serveur,msgQuit;
		int port;
        QTabWidget *tab;
		QMap<QString,QTextEdit *> conversations;
		QSystemTrayIcon *tray;

		bool updateUsers;

		QString parseCommande(QString comm,bool serveur=false);

		QWidget *parent;

	Q_SIGNALS:
		void pseudoChanged(QString newPseudo);
		void joinTab();
        void tabJoined();

	public Q_SLOTS:
		void readServeur();
		void errorSocket(QAbstractSocket::SocketError);
        void connected();
        void joins();
		void sendData(QString txt);
		void join(QString chan);
        void leave(QString chan);
        void ecrire(QString txt,QString destChan="",QString msgTray="");
        void updateUsersList(QString chan="",QString message="");

};

#endif
