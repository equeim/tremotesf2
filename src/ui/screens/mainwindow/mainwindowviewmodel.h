// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWVIEWMODEL_H
#define TREMOTESF_MAINWINDOWVIEWMODEL_H

#include <QObject>
#include <QStringList>

#include "rpc/trpc.h"

class QByteArray;
class QDragEnterEvent;
class QDropEvent;
class QString;
class QSystemTrayIcon;
class QTimer;

namespace tremotesf {

    class MainWindowViewModel final : public QObject {
        Q_OBJECT

    public:
        MainWindowViewModel(QStringList&& commandLineFiles, QStringList&& commandLineUrls, QObject* parent = nullptr);

        Rpc* rpc() { return &mRpc; };

        static void processDragEnterEvent(QDragEnterEvent* event);
        void processDropEvent(QDropEvent* event);
        void pasteShortcutActivated();

        void setupNotificationsController(QSystemTrayIcon* trayIcon);

        enum class StartupActionResult { ShowAddServerDialog, DoNothing };
        StartupActionResult performStartupAction();

    private:
        Rpc mRpc{};
        QStringList mPendingFilesToOpen{};
        QStringList mPendingUrlsToOpen{};
        QTimer* delayedTorrentAddMessageTimer{};

        void addTorrents(const QStringList& files, const QStringList& urls, bool showDelayedMessageWithDelay = false);

    signals:
        void showWindow(const QByteArray& newStartupNotificationId);
        void showAddTorrentDialogs(const QStringList& files, const QStringList& urls);
        void showDelayedTorrentAddMessage(const QStringList& torrents);
    };
}

#endif // TREMOTESF_MAINWINDOWVIEWMODEL_H
