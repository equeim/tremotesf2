#ifndef TREMOTESF_NOTIFICATIONSCONTROLLER_H
#define TREMOTESF_NOTIFICATIONSCONTROLLER_H

#include <QObject>

class QSystemTrayIcon;

namespace tremotesf {
    class NotificationsController : public QObject {
        Q_OBJECT
    public:
        static NotificationsController* createInstance(QSystemTrayIcon* trayIcon, QObject* parent = nullptr);

        void showFinishedTorrentsNotification(const QStringList& torrentNames);
        void showAddedTorrentsNotification(const QStringList& torrentNames);

        virtual void showNotification(const QString& title, const QString& message);

    protected:
        explicit NotificationsController(QSystemTrayIcon* trayIcon, QObject* parent = nullptr);

        void fallbackToSystemTrayIcon(const QString& title, const QString& message);

    private:
        void showTorrentsNotification(const QString& title, const QStringList& torrentNames);

        QSystemTrayIcon* mTrayIcon{};

    signals:
        void notificationClicked();
    };
}

#endif // TREMOTESF_NOTIFICATIONSCONTROLLER_H
