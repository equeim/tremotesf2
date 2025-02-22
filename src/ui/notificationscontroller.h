// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_NOTIFICATIONSCONTROLLER_H
#define TREMOTESF_NOTIFICATIONSCONTROLLER_H

#include <optional>
#include <QObject>

class QSystemTrayIcon;

namespace tremotesf {
    class Rpc;

    class NotificationsController : public QObject {
        Q_OBJECT

    public:
        static NotificationsController*
        createInstance(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent = nullptr);

    protected:
        explicit NotificationsController(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent = nullptr);

        virtual void showNotification(const QString& title, const QString& message);
        void fallbackToSystemTrayIcon(const QString& title, const QString& message);

    private:
        void onConnected(const Rpc* rpc);
        void onDisconnected(const Rpc* rpc);
        void showFinishedTorrentsNotification(const QStringList& torrentNames);
        void showAddedTorrentsNotification(const QStringList& torrentNames);
        void showTorrentsNotification(const QString& title, const QStringList& torrentNames);

        QSystemTrayIcon* mTrayIcon{};

    signals:
        void notificationClicked(const std::optional<QByteArray>& windowActivationToken);
    };
}

#endif // TREMOTESF_NOTIFICATIONSCONTROLLER_H
