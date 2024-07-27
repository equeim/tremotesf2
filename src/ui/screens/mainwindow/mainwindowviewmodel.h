// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MAINWINDOWVIEWMODEL_H
#define TREMOTESF_MAINWINDOWVIEWMODEL_H

#include <QObject>
#include <QStringList>

#include "rpc/rpc.h"
#include "coroutines/scope.h"

class QByteArray;
class QDragEnterEvent;
class QDropEvent;
class QMimeData;
class QString;
class QSystemTrayIcon;

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
        void acceptedFileDialog(QStringList files);

        void setupNotificationsController(QSystemTrayIcon* trayIcon);

        enum class StartupActionResult { ShowAddServerDialog, DoNothing };
        StartupActionResult performStartupAction();

    private:
        Rpc mRpc{};
        CoroutineScope mAddingTorrentsCoroutineScope{};

        bool addTorrentsFromClipboard(bool onlyUrls = false);
        bool addTorrentsFromMimeData(const QMimeData* mimeData, bool onlyUrls);

        void addTorrentFilesWithoutDialog(const QStringList& files);
        void addTorrentLinksWithoutDialog(const QStringList& urls);
        std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>>
        separateTorrentsThatAlreadyExistForLinks(QStringList& urls);

        Coroutine<>
        addTorrents(QStringList files, QStringList urls, std::optional<QByteArray> windowActivationToken = {});

    signals:
        void showWindow(const std::optional<QByteArray>& windowActivationToken);
        void showAddTorrentDialogs(
            const QStringList& files, const QStringList& urls, const std::optional<QByteArray>& windowActivationToken
        );
        void askForMergingTrackers(
            const std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>>& existingTorrents,
            const std::optional<QByteArray>& windowActivationToken
        );
        void showDelayedTorrentAddMessage(const QStringList& torrents);
        void hideDelayedTorrentAddMessage();
    };
}

#endif // TREMOTESF_MAINWINDOWVIEWMODEL_H
