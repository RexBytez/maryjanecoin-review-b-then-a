#include "splashscreen.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <cmath>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f)
    : QSplashScreen(pixmap, f), messageAlignment(Qt::AlignLeft), messageColor(Qt::black)
{
    messageText = "";
}

void SplashScreen::showMessageWithShadow(const QString &msg, Qt::Alignment align, const QColor &color)
{
    messageText = msg;
    messageAlignment = align;
    messageColor = color;

    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{

    QSplashScreen::paintEvent(event);

    if (messageText.isEmpty())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QFont font = painter.font();
    font.setBold(true);

    int currentSize = font.pointSize();
    if (currentSize > 0) {
        font.setPointSize(currentSize + 1);
    } else {

        int pixelSize = font.pixelSize();
        if (pixelSize > 0) {
            font.setPixelSize(pixelSize + 1);
        } else {

            font.setPointSize(10);
        }
    }
    painter.setFont(font);

    QFontMetrics fm(font);
    QRect widgetRect = rect();

    int x = 0, y = 0;

    int textWidth = fm.width(messageText);

    if (messageAlignment & Qt::AlignHCenter) {
        x = (widgetRect.width() - textWidth) / 2;
    } else if (messageAlignment & Qt::AlignRight) {
        x = widgetRect.width() - textWidth - 12;
    } else {
        x = 12;
    }

    if (messageAlignment & Qt::AlignVCenter) {
        y = (widgetRect.height() + fm.height()) / 2;
    } else if (messageAlignment & Qt::AlignBottom) {
        y = widgetRect.height() - 12;
    } else {
        y = fm.height() + 12;
    }

    QColor lightGreen(144, 238, 144);
    painter.setPen(QPen(lightGreen, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawText(x, y, messageText);
}
