#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include "bitcoin.h"

#include "bitcoingui.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "splashscreen.h"

#include "init.h"
#include "ui_interface.h"
#include "qtipcserver.h"

#include <QApplication>
#include <QMessageBox>
#include <QTextCodec>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>

#if defined(BITCOIN_NEED_QT_PLUGINS) && !defined(_BITCOIN_QT_PLUGINS_INCLUDED)
#define _BITCOIN_QT_PLUGINS_INCLUDED
#define __INSURE__
#include <QtPlugin>
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
Q_IMPORT_PLUGIN(qtaccessiblewidgets)
#endif

#if defined(QT_STATIC)
#include <QtPlugin>
#if defined(QT_QPA_PLATFORM_XCB)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_WINDOWS)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin);
#elif defined(QT_QPA_PLATFORM_COCOA)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
Q_IMPORT_PLUGIN(QMacStylePlugin);
#elif defined(QT_QPA_PLATFORM_ANDROID)
Q_IMPORT_PLUGIN(QAndroidPlatformIntegrationPlugin)
#endif
#endif

static BitcoinGUI *guiref;
static SplashScreen *splashref;

static void ThreadSafeMessageBox(const std::string& message, const std::string& caption, int style)
{

    if(guiref)
    {
        bool modal = (style & CClientUIInterface::MODAL);

        QMetaObject::invokeMethod(guiref, "error",
                                   modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                                   Q_ARG(QString, QString::fromStdString(caption)),
                                   Q_ARG(QString, QString::fromStdString(message)),
                                   Q_ARG(bool, modal));
    }
    else
    {
        printf("%s: %s\n", caption.c_str(), message.c_str());
        fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
    }
}

static bool ThreadSafeAskFee(int64_t nFeeRequired, const std::string& strCaption)
{
    if(!guiref)
        return false;
    if(nFeeRequired < MIN_TX_FEE || nFeeRequired <= nTransactionFee || fDaemon)
        return true;
    bool payFee = false;

    QMetaObject::invokeMethod(guiref, "askFee", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(qint64, nFeeRequired),
                               Q_ARG(bool*, &payFee));

    return payFee;
}

static void ThreadSafeHandleURI(const std::string& strURI)
{
    if(!guiref)
        return;

    QMetaObject::invokeMethod(guiref, "handleURI", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(QString, QString::fromStdString(strURI)));
}

static void InitMessage(const std::string &message)
{
    if(splashref)
    {
        splashref->showMessageWithShadow(QString::fromStdString(message), Qt::AlignBottom|Qt::AlignHCenter, QColor(0,0,0));

        QApplication::instance()->processEvents();
    }
}

static void QueueShutdown()
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}

static std::string Translate(const char* psz)
{
    return QCoreApplication::translate("bitcoin-core", psz).toStdString();
}

static void handleRunawayException(std::exception *e)
{
    PrintExceptionContinue(e, "Runaway exception");
    QMessageBox::critical(0, "Runaway exception", BitcoinGUI::tr("A fatal error occurred. MaryJaneCoin can no longer continue safely and will quit.") + QString("\n\n") + QString::fromStdString(strMiscWarning));
    exit(1);
}

#ifndef BITCOIN_QT_TEST
int GuiMain(int argc, char *argv[])
{

    ipcScanRelay(argc, argv);

#if QT_VERSION < 0x050000

    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());
#endif

    Q_INIT_RESOURCE(bitcoin);
    QApplication app(argc, argv);

    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));

    ParseParameters(argc, argv);

    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {

        QMessageBox::critical(0, "MaryJaneCoin",
                              QString("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(mapArgs["-datadir"])));
        return 1;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);

    app.setOrganizationName("MaryJaneCoin");

    if(GetBoolArg("-testnet"))
        app.setApplicationName("MaryJaneCoin-Qt-testnet");
    else
        app.setApplicationName("MaryJaneCoin-Qt");

    OptionsModel optionsModel;

    QString lang_territory = QString::fromStdString(GetArg("-lang", QLocale::system().name().toStdString()));
    QString lang = lang_territory;

    lang.truncate(lang_territory.lastIndexOf('_'));

    QTranslator qtTranslatorBase, qtTranslator, translatorBase, translator;

    if (qtTranslatorBase.load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTranslatorBase);

    if (qtTranslator.load("qt_" + lang_territory, QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTranslator);

    if (translatorBase.load(lang, ":/translations/"))
        app.installTranslator(&translatorBase);

    if (translator.load(lang_territory, ":/translations/"))
        app.installTranslator(&translator);

    uiInterface.ThreadSafeMessageBox.connect(ThreadSafeMessageBox);
    uiInterface.ThreadSafeAskFee.connect(ThreadSafeAskFee);
    uiInterface.ThreadSafeHandleURI.connect(ThreadSafeHandleURI);
    uiInterface.InitMessage.connect(InitMessage);
    uiInterface.QueueShutdown.connect(QueueShutdown);
    uiInterface.Translate.connect(Translate);

    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        GUIUtil::HelpMessageBox help;
        help.showOrPrint();
        return 1;
    }

    SplashScreen splash(QPixmap(":/images/splash"), 0);
    if (GetBoolArg("-splash", true) && !GetBoolArg("-min"))
    {
        splash.show();
        splashref = &splash;
    }

    app.processEvents();

    app.setQuitOnLastWindowClosed(false);

    try
    {

        if (GUIUtil::GetStartOnSystemStartup())
            GUIUtil::SetStartOnSystemStartup(true);

        BitcoinGUI window;
        guiref = &window;
        if(AppInit2())
        {
            {

                optionsModel.Upgrade();

                if (splashref)
                    splash.finish(&window);

                ClientModel clientModel(&optionsModel);
                WalletModel walletModel(pwalletMain, &optionsModel);

                window.setClientModel(&clientModel);
                window.setWalletModel(&walletModel);

                if(GetBoolArg("-min"))
                {
                    window.showMinimized();
                }
                else
                {
                    window.show();
                }

                ipcInit(argc, argv);

                app.exec();

                window.hide();
                window.setClientModel(0);
                window.setWalletModel(0);
                guiref = 0;
            }

            Shutdown(NULL);
        }
        else
        {
            return 1;
        }
    } catch (std::exception& e) {
        handleRunawayException(&e);
    } catch (...) {
        handleRunawayException(NULL);
    }
    return 0;
}
#endif
