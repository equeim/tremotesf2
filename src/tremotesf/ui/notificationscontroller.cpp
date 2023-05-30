// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

#include <QCoreApplication>
#include <QSystemTrayIcon>

#include "libtremotesf/log.h"

namespace tremotesf {
    void NotificationsController::showNotification(const QString& title, const QString& message) {
        fallbackToSystemTrayIcon(title, message);
    }

    NotificationsController::NotificationsController(QSystemTrayIcon* trayIcon, QObject* parent)
        : QObject(parent), mTrayIcon(trayIcon) {
        QObject::connect(mTrayIcon, &QSystemTrayIcon::messageClicked, this, [this] {
            logInfo("NotificationsController: notification clicked");
            emit notificationClicked();
        });
    }

    void NotificationsController::fallbackToSystemTrayIcon(const QString& title, const QString& message) {
        if (mTrayIcon->isVisible()) {
            logInfo("NotificationsController: executing QSystemTrayIcon::showMessage()");
            mTrayIcon->showMessage(title, message, QSystemTrayIcon::Information, 0);
        } else {
            logWarning("NotificationsController: system tray icon is not visible, don't show notification");
        }
    }

    void NotificationsController::showFinishedTorrentsNotification(const QStringList& torrentNames) {
        showTorrentsNotification(
            torrentNames.size() == 1
                //: Notification title
                ? qApp->translate("tremotesf", "Torrent finished")
                //: Notification title, %Ln is number of finished torrents
                : qApp->translate("tremotesf", "%Ln torrents finished", nullptr, static_cast<int>(torrentNames.size())),
            torrentNames
        );
    }

    void NotificationsController::showAddedTorrentsNotification(const QStringList& torrentNames) {
        showTorrentsNotification(
            torrentNames.size() == 1
                //: Notification title
                ? qApp->translate("tremotesf", "Torrent added")
                //: Notification title, %Ln is number of added torrents
                : qApp->translate("tremotesf", "%Ln torrents added", nullptr, static_cast<int>(torrentNames.size())),
            torrentNames
        );
    }

    void NotificationsController::showTorrentsNotification(const QString& title, const QStringList& torrentNames) {
        if (torrentNames.size() == 1) {
            showNotification(title, torrentNames.first());
        } else {
            auto names = torrentNames;
            for (auto& name : names) {
                name.prepend(u"\u2022 ");
            }
            showNotification(title, names.join('\n'));
        }
    }
}
