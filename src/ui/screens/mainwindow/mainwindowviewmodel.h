// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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
    class TorrentMetainfoFile;

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

        Coroutine<std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>>>
        separateTorrentsThatAlreadyExistForFiles(QStringList& files);
        Coroutine<> parseTorrentFile(QString filePath, std::vector<std::pair<QString, TorrentMetainfoFile>>& output);
        void addTorrentFilesWithoutDialog(const QStringList& files);

        std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>>
        separateTorrentsThatAlreadyExistForLinks(QStringList& urls);
        void addTorrentLinksWithoutDialog(QStringList urls);

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
        void showDelayedTorrentAddDialog(
            const QStringList& torrents, const std::optional<QByteArray>& windowActivationToken
        );
    };
}

#endif // TREMOTESF_MAINWINDOWVIEWMODEL_H
