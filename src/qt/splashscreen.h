#ifndef BITCOIN_QT_SPLASHSCREEN_H
#define BITCOIN_QT_SPLASHSCREEN_H

#include <QSplashScreen>
#include <QString>
#include <QColor>

class QPaintEvent;

class SplashScreen : public QSplashScreen
{
public:
    SplashScreen(const QPixmap &pixmap = QPixmap(), Qt::WindowFlags f = Qt::WindowFlags());

    void paintEvent(QPaintEvent *event) override;

    void showMessageWithShadow(const QString &message, Qt::Alignment alignment = Qt::AlignLeft, const QColor &color = Qt::black);

protected:
    QString messageText;
    Qt::Alignment messageAlignment;
    QColor messageColor;
};

#endif
