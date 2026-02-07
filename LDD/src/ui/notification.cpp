#include "notification.h"

#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QSystemTrayIcon>

void showSystemNotification(const QString &title, const QString &message)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::information(nullptr, title, message);
        return;
    }

    static QSystemTrayIcon *tray = nullptr;
    if (!tray) {
        QIcon icon = qApp ? qApp->windowIcon() : QIcon();
        if (icon.isNull()) {
            icon = QIcon(":/images/1.png");
        }
        tray = new QSystemTrayIcon(icon, qApp);
        tray->setVisible(true);
    }

    tray->showMessage(title, message, QSystemTrayIcon::Information, 5000);
}
