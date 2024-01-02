// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

#include <QCoreApplication>
#include <QSystemTrayIcon>

#include "log/log.h"
#include "rpc/rpc.h"
#include "rpc/servers.h"
#include "settings.h"

namespace tremotesf {
    void NotificationsController::showNotification(const QString& title, const QString& message) {
        fallbackToSystemTrayIcon(title, message);
    }

    NotificationsController::NotificationsController(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent)
        : QObject(parent), mTrayIcon(trayIcon) {
        QObject::connect(mTrayIcon, &QSystemTrayIcon::messageClicked, this, [this] {
            logInfo("NotificationsController: notification clicked");
            emit notificationClicked();
        });

        QObject::connect(rpc, &Rpc::connectedChanged, this, [rpc, this] {
            if (rpc->isConnected()) {
                onConnected(rpc);
            } else {
                onDisconnected(rpc);
            }
        });

        QObject::connect(rpc, &Rpc::torrentAdded, this, [this](const auto* torrent) {
            if (Settings::instance()->notificationOnAddingTorrent()) {
                showAddedTorrentsNotification({torrent->data().name});
            }
        });

        QObject::connect(rpc, &Rpc::torrentFinished, this, [this](const auto* torrent) {
            if (Settings::instance()->notificationOfFinishedTorrents()) {
                showFinishedTorrentsNotification({torrent->data().name});
            }
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

    void NotificationsController::onConnected(const Rpc* rpc) {
        const bool notifyOnAdded = Settings::instance()->notificationsOnAddedTorrentsSinceLastConnection();
        const bool notifyOnFinished = Settings::instance()->notificationsOnFinishedTorrentsSinceLastConnection();
        if (!notifyOnAdded && !notifyOnFinished) {
            return;
        }
        const LastTorrents lastTorrents(Servers::instance()->currentServerLastTorrents());
        if (!lastTorrents.saved) {
            return;
        }
        QStringList addedNames{};
        QStringList finishedNames{};
        for (const auto& torrent : rpc->torrents()) {
            const QString hashString(torrent->data().hashString);
            const auto found = std::find_if(
                lastTorrents.torrents.cbegin(),
                lastTorrents.torrents.cend(),
                [&hashString](const auto& torrent) { return torrent.hashString == hashString; }
            );
            if (found == lastTorrents.torrents.cend()) {
                if (notifyOnAdded) {
                    addedNames.push_back(torrent->data().name);
                }
            } else {
                if (notifyOnFinished && !found->finished && torrent->data().isFinished()) {
                    finishedNames.push_back(torrent->data().name);
                }
            }
        }
        if (notifyOnAdded && !addedNames.isEmpty()) {
            showAddedTorrentsNotification(addedNames);
        }
        if (notifyOnFinished && !finishedNames.isEmpty()) {
            showFinishedTorrentsNotification(finishedNames);
        }
    }

    void NotificationsController::onDisconnected(const Rpc* rpc) {
        const auto& status = rpc->status();
        if ((status.error != Rpc::Error::NoError) && Settings::instance()->notificationOnDisconnecting()) {
            showNotification(
                //: Notification title when disconnected from server
                qApp->translate("tremotesf", "Disconnected"),
                status.toString()
            );
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
