#include "chatwindow.h"
#include <qt/forms/ui_chatwindow.h>

ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::ChatWindowClass)
{
    ui->setupUi(this);
    setFixedSize(760,600);
    ui->splitter->hide();

	connect(ui->buttonConnect, SIGNAL(clicked()), this, SLOT(connecte()));

	connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui->actionCloseTab, SIGNAL(triggered()), this, SLOT(closeTab()));

	connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(sendCommande()));

    connect(ui->disconnect, SIGNAL(clicked()), this, SLOT(disconnectFromServer()));
	connect(ui->tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)) );
	connect(ui->tab, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClosing(int)) );

}

void ChatWindow::tabChanged(int index)
{
	if(index!=0 && joining == false)
		currentTab()->updateUsersList(ui->tab->tabText(index));
}

void ChatWindow::tabClosing(int index)
{
	currentTab()->leave(ui->tab->tabText(index));
}

void ChatWindow::disconnectFromServer() {

    QMapIterator<QString, Serveur*> i(serveurs);

    while(i.hasNext())
    {
        i.next();
        QMapIterator<QString, QTextEdit*> i2(i.value()->conversations);
        while(i2.hasNext())
        {
            i2.next();
            i.value()->sendData("QUIT "+i2.key() + " ");
        }
    }

    ui->splitter->hide();
    ui->hide3->show();

}

Serveur *ChatWindow::currentTab()
{
	QString tooltip=ui->tab->tabToolTip(ui->tab->currentIndex());
	return serveurs[tooltip];

}

void ChatWindow::closeTab()
{
	QString tooltip=ui->tab->tabToolTip(ui->tab->currentIndex());
	QString txt=ui->tab->tabText(ui->tab->currentIndex());

	if(txt==tooltip)
	{
		QMapIterator<QString, QTextEdit*> i(serveurs[tooltip]->conversations);

		int count=ui->tab->currentIndex()+1;

		while(i.hasNext())
		{
			i.next();
			ui->tab->removeTab(count);
		}

		currentTab()->abort();
		ui->tab->removeTab(ui->tab->currentIndex());
	}
	else
	{

        ui->tab->removeTab(ui->tab->currentIndex());
		currentTab()->conversations.remove(txt);
	}
}

void ChatWindow::sendCommande()
{
	QString tooltip=ui->tab->tabToolTip(ui->tab->currentIndex());
	QString txt=ui->tab->tabText(ui->tab->currentIndex());
	if(txt==tooltip)
	{
		currentTab()->sendData(currentTab()->parseCommande(ui->lineEdit->text(),true) );
	}
	else
	{
		currentTab()->sendData(currentTab()->parseCommande(ui->lineEdit->text()) );
	}
	ui->lineEdit->clear();
	ui->lineEdit->setFocus();
}

void ChatWindow::tabJoined()
{
	joining=true;
}
void ChatWindow::tabJoining()
{
	joining=false;
}

void ChatWindow::connecte()
{

    ui->splitter->show();
	Serveur *serveur=new Serveur;
    QTextEdit *textEdit=new QTextEdit;
    ui->hide3->hide();

    ui->tab->addTab(textEdit,"Console/PM");
    ui->tab->setTabToolTip(ui->tab->count()-1,"chat.freenode.net");

    for (int i = ui->tab->count(); i > 1; --i) {
       ui->tab->removeTab(0);
    }

    serveurs.insert("chat.freenode.net",serveur);

	serveur->pseudo=ui->editPseudo->text();
    serveur->serveur="chat.freenode.net";
    serveur->port=6667;
	serveur->affichage=textEdit;
    serveur->tab=ui->tab;
	serveur->userList=ui->userView;
	serveur->parent=this;

	textEdit->setReadOnly(true);

	connect(serveur, SIGNAL(joinTab()),this, SLOT(tabJoined() ));
	connect(serveur, SIGNAL(tabJoined()),this, SLOT(tabJoining() ));

    serveur->connectToHost("chat.freenode.net",6667);

	ui->tab->setCurrentIndex(ui->tab->count()-1);
}

void ChatWindow::closeEvent(QCloseEvent *event)
{
	(void) event;

	QMapIterator<QString, Serveur*> i(serveurs);

	while(i.hasNext())
	{
		i.next();
		QMapIterator<QString, QTextEdit*> i2(i.value()->conversations);
		while(i2.hasNext())
		{
			i2.next();
            i.value()->sendData("QUIT "+i2.key() + " ");
		}
	}
}
void ChatWindow ::setModel(ClientModel *model)
{
    this->model = model;
}

ChatWindow::~ChatWindow()
{
    delete ui;
    QMapIterator<QString, Serveur*> i(serveurs);

    while(i.hasNext())
    {
        i.next();
        QMapIterator<QString, QTextEdit*> i2(i.value()->conversations);
        while(i2.hasNext())
        {
            i2.next();
            i.value()->sendData("QUIT "+i2.key() + " ");
        }
    }
}
