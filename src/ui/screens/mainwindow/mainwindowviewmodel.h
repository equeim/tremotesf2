// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWVIEWMODEL_H
#define TREMOTESF_MAINWINDOWVIEWMODEL_H

#include <QObject>
#include <QStringList>

#include "rpc/rpc.h"

class QByteArray;
class QDragEnterEvent;
class QDropEvent;
class QMimeData;
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
        void triggeredAddTorrentLinkAction();

        void setupNotificationsController(QSystemTrayIcon* trayIcon);

        enum class StartupActionResult { ShowAddServerDialog, DoNothing };
        StartupActionResult performStartupAction();

        void addTorrentFilesWithoutDialog(const QStringList& files);
        void addTorrentLinksWithoutDialog(const QStringList& urls);

    private:
        Rpc mRpc{};
        QStringList mPendingFilesToOpen{};
        QStringList mPendingUrlsToOpen{};
        QTimer* delayedTorrentAddMessageTimer{};

        bool addTorrentsFromClipboard(bool onlyUrls = false);
        bool addTorrentsFromMimeData(const QMimeData* mimeData, bool onlyUrls);

        void addTorrents(
            const QStringList& files,
            const QStringList& urls,
            const std::optional<QByteArray>& windowActivationToken = {}
        );

    signals:
        void showWindow(const std::optional<QByteArray>& windowActivationToken);
        void showAddTorrentDialogs(
            const QStringList& files, const QStringList& urls, const std::optional<QByteArray>& windowActivationToken
        );
        void showDelayedTorrentAddMessage(const QStringList& torrents);
    };
}

#endif // TREMOTESF_MAINWINDOWVIEWMODEL_H
