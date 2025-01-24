// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindowviewmodel.h"

#include <chrono>

#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGuiApplication>
#include <QMimeData>
#include <QUrl>

#include <fmt/ranges.h>

#include "coroutines/qobjectsignal.h"
#include "coroutines/timer.h"
#include "coroutines/threadpool.h"
#include "coroutines/waitall.h"
#include "log/log.h"
#include "ipc/ipcserver.h"
#include "rpc/servers.h"
#include "ui/screens/addtorrent/addtorrenthelpers.h"
#include "ui/screens/addtorrent/droppedtorrents.h"
#include "ui/notificationscontroller.h"
#include "magnetlinkparser.h"
#include "settings.h"
#include "stdutils.h"
#include "torrentfileparser.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)
SPECIALIZE_FORMATTER_FOR_QDEBUG(Qt::DropActions)

namespace tremotesf {
    namespace {
        std::string formatDropEvent(const QDropEvent* event) {
            const auto mime = event->mimeData();
            return fmt::format(
                R"(
    proposedAction = {}
    possibleActions = {}
    formats = {}
    urls = {}
    text = {})",
                event->proposedAction(),
                event->possibleActions(),
                mime->formats(),
                mime->urls(),
                mime->text()
            );
        }

        std::string formatMimeData(const QMimeData* mime) {
            return fmt::format(
                R"(
    formats = {}
    urls = {}
    text = {})",
                mime->formats(),
                mime->urls(),
                mime->text()
            );
        }

        using namespace std::chrono_literals;
        constexpr auto initialDelayedTorrentAddMessageDelay = 500ms;
    }

    MainWindowViewModel::MainWindowViewModel(
        QStringList&& commandLineFiles, QStringList&& commandLineUrls, QObject* parent
    )
        : QObject(parent) {
        if (!commandLineFiles.isEmpty() || !commandLineUrls.isEmpty()) {
            QMetaObject::invokeMethod(
                this,
                [this, files = std::move(commandLineFiles), urls = std::move(commandLineUrls)]() mutable {
                    mAddingTorrentsCoroutineScope.launch(addTorrents(std::move(files), std::move(urls)));
                },
                Qt::QueuedConnection
            );
        }

        const auto* const ipcServer = IpcServer::createInstance(this);
        QObject::connect(
            ipcServer,
            &IpcServer::windowActivationRequested,
            this,
            [=, this](const auto& activationToken) { emit showWindow(activationToken); }
        );

        QObject::connect(
            ipcServer,
            &IpcServer::torrentsAddingRequested,
            this,
            [=, this](const auto& files, const auto& urls, const auto& activationToken) {
                mAddingTorrentsCoroutineScope.launch(addTorrents(files, urls, activationToken));
            }
        );

        QObject::connect(Servers::instance(), &Servers::currentServerChanged, this, [this] {
            if (Servers::instance()->hasServers()) {
                mRpc.setConnectionConfiguration(Servers::instance()->currentServer().connectionConfiguration);
                mRpc.connect();
            } else {
                mRpc.resetConnectionConfiguration();
            }
        });

        QObject::connect(&mRpc, &Rpc::aboutToDisconnect, this, [this] {
            Servers::instance()->saveCurrentServerLastTorrents(&mRpc);
        });
    }

    void MainWindowViewModel::processDragEnterEvent(QDragEnterEvent* event) {
        debug().log("MainWindowViewModel: processing QDragEnterEvent");
        debug().log("MainWindowViewModel: event: {}", formatDropEvent(event));
        const auto dropped = DroppedTorrents(event->mimeData());
        if (dropped.isEmpty()) {
            debug().log("MainWindowViewModel: not accepting QDragEnterEvent");
            return;
        }
        info().log("MainWindowViewModel: accepting QDragEnterEvent");
        if (event->possibleActions().testFlag(Qt::CopyAction)) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    }

    void MainWindowViewModel::processDropEvent(QDropEvent* event) {
        debug().log("MainWindowViewModel: processing QDropEvent");
        debug().log("MainWindowViewModel: event: {}", formatDropEvent(event));
        auto dropped = DroppedTorrents(event->mimeData());
        if (dropped.isEmpty()) {
            warning().log("Dropped torrents are empty");
            return;
        }
        info().log("MainWindowViewModel: accepting QDropEvent");
        event->acceptProposedAction();
        mAddingTorrentsCoroutineScope.launch(addTorrents(std::move(dropped.files), std::move(dropped.urls)));
    }

    void MainWindowViewModel::pasteShortcutActivated() {
        debug().log("MainWindowViewModel: pasteShortcutActivated() called");
        addTorrentsFromClipboard();
    }

    void MainWindowViewModel::triggeredAddTorrentLinkAction() {
        debug().log("MainWindowViewModel: triggeredAddTorrentLinkAction() called");
        if (Settings::instance()->get_fillTorrentLinkFromClipboard() && addTorrentsFromClipboard(true)) {
            return;
        }
        emit showAddTorrentDialogs({}, {QString{}}, {});
    }

    void MainWindowViewModel::acceptedFileDialog(QStringList files) {
        debug().log("MainWindowViewModel: acceptedFileDialog() called with: files = {}", files);
        mAddingTorrentsCoroutineScope.launch(addTorrents(std::move(files), {}));
    }

    void MainWindowViewModel::setupNotificationsController(QSystemTrayIcon* trayIcon) {
        const auto controller = NotificationsController::createInstance(trayIcon, &mRpc, this);
        QObject::connect(controller, &NotificationsController::notificationClicked, this, [this] {
            emit showWindow({});
        });
    }

    MainWindowViewModel::StartupActionResult MainWindowViewModel::performStartupAction() {
        if (!Servers::instance()->hasServers()) {
            return StartupActionResult::ShowAddServerDialog;
        }
        mRpc.setConnectionConfiguration(Servers::instance()->currentServer().connectionConfiguration);
        if (Settings::instance()->get_connectOnStartup()) {
            mRpc.connect();
        }
        return StartupActionResult::DoNothing;
    }

    bool MainWindowViewModel::addTorrentsFromClipboard(bool onlyUrls) {
        const auto* const mimeData = QGuiApplication::clipboard()->mimeData();
        if (!mimeData) {
            warning().log("MainWindowViewModel: clipboard data is null");
            return false;
        }
        debug().log("MainWindowViewModel: clipboard data: {}", formatMimeData(mimeData));
        if (addTorrentsFromMimeData(mimeData, onlyUrls)) {
            return true;
        }
        debug().log("MainWindowViewModel: ignoring clipboard data");
        return false;
    }

    bool MainWindowViewModel::addTorrentsFromMimeData(const QMimeData* mimeData, bool onlyUrls) {
        auto dropped = DroppedTorrents(mimeData);
        if (dropped.isEmpty()) {
            return false;
        }
        mAddingTorrentsCoroutineScope.launch(
            addTorrents(onlyUrls ? QStringList{} : std::move(dropped.files), std::move(dropped.urls))
        );
        return true;
    }

    Coroutine<std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>>>
    MainWindowViewModel::separateTorrentsThatAlreadyExistForFiles(
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
        QStringList& files
    ) {
        std::vector<std::pair<QString, TorrentMetainfoFile>> torrentFiles{};
        torrentFiles.reserve(static_cast<size_t>(files.size()));
        co_await waitAll(toContainer<std::vector>(files | std::views::transform([&](const QString& filePath) {
                                                      return parseTorrentFile(filePath, torrentFiles);
                                                  })));

        std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>> existingTorrents{};

        for (auto& [filePath, torrentFile] : torrentFiles) {
            auto* const torrent = mRpc.torrentByHash(torrentFile.infoHashV1);
            if (torrent) {
                existingTorrents.emplace_back(torrent, std::move(torrentFile.trackers));
                files.removeOne(filePath);
            }
        }

        co_return existingTorrents;
    }

    Coroutine<> MainWindowViewModel::parseTorrentFile(
        QString filePath,
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
        std::vector<std::pair<QString, TorrentMetainfoFile>>& output
    ) {
        try {
            info().log("Parsing torrent file {}", filePath);
            auto torrentFile = co_await runOnThreadPool(&tremotesf::parseTorrentFile, filePath);
            info().log("Parsed {}, result = {}", filePath, torrentFile);
            output.emplace_back(std::move(filePath), std::move(torrentFile));
        } catch (const bencode::Error& e) {
            warning().logWithException(e, "Failed to parse torrent file {}", filePath);
        }
    }

    void MainWindowViewModel::addTorrentFilesWithoutDialog(const QStringList& files) {
        if (files.isEmpty()) return;
        const auto parameters = getAddTorrentParameters(&mRpc);
        for (const auto& filePath : files) {
            mRpc.addTorrentFile(
                filePath,
                parameters.downloadDirectory,
                {},
                {},
                {},
                {},
                parameters.priority,
                parameters.startAfterAdding,
                parameters.deleteTorrentFile ? (parameters.moveTorrentFileToTrash ? Rpc::DeleteFileMode::MoveToTrash
                                                                                  : Rpc::DeleteFileMode::Delete)
                                             : Rpc::DeleteFileMode::No

            );
        }
    }

    std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>>
    MainWindowViewModel::separateTorrentsThatAlreadyExistForLinks(QStringList& urls) {
        std::vector<std::pair<Torrent*, std::vector<std::set<QString>>>> existingTorrents{};
        const auto toErase = std::ranges::remove_if(urls, [&](const QString& url) {
            try {
                info().log("Parsing {} as a magnet link", url);
                auto magnetLink = parseMagnetLink(QUrl(url));
                info().log("Parsed, result = {}", magnetLink);
                auto* const torrent = mRpc.torrentByHash(magnetLink.infoHashV1);
                if (torrent) {
                    existingTorrents.emplace_back(torrent, std::move(magnetLink.trackers));
                    return true;
                }
            } catch (const std::runtime_error& e) {
                warning().logWithException(e, "Failed to parse {} as a magnet link", url);
            }
            return false;
        });
        if (!toErase.empty()) {
            urls.erase(toErase.begin(), toErase.end());
        }
        return existingTorrents;
    }

    void MainWindowViewModel::addTorrentLinksWithoutDialog(QStringList urls) {
        if (urls.isEmpty()) return;
        auto parameters = getAddTorrentParameters(&mRpc);
        mRpc.addTorrentLinks(
            std::move(urls),
            std::move(parameters.downloadDirectory),
            parameters.priority,
            parameters.startAfterAdding
        );
    }

    Coroutine<> MainWindowViewModel::addTorrents(
        QStringList files, QStringList urls, std::optional<QByteArray> windowActivationToken
    ) {
        info().log("MainWindowViewModel: addTorrents() called");
        info().log("MainWindowViewModel: files = {}", files);
        info().log("MainWindowViewModel: urls = {}", urls);

        if (!mRpc.isConnected()) {
            info().log("Postponing opening torrents until connected to server");
            if (mRpc.connectionState() == RpcConnectionState::Connecting) {
                info().log("We are already connecting, wait a bit before showing message");
                co_await waitAny(
                    []() -> Coroutine<> { co_await waitFor(initialDelayedTorrentAddMessageDelay); }(),
                    [](Rpc* rpc) -> Coroutine<> { co_await waitForSignal(rpc, &Rpc::connectedChanged); }(&mRpc)
                );
            }
            if (!mRpc.isConnected()) {
                info().log("Showing delayed torrent adding message");
                emit showDelayedTorrentAddMessage(files + urls);
                co_await waitForSignal(&mRpc, &Rpc::connectedChanged);
            }
        }

        const auto* const settings = Settings::instance();

        if (settings->get_showMainWindowWhenAddingTorrent()) {
            emit showWindow(windowActivationToken);
            windowActivationToken.reset();
        }

        emit hideDelayedTorrentAddMessage();

        // We can parse magnet links immediately so check whether torrents exist event if we show dialogs
        if (const auto existingTorrents = separateTorrentsThatAlreadyExistForLinks(urls); !existingTorrents.empty()) {
            emit askForMergingTrackers(existingTorrents, windowActivationToken);
            windowActivationToken.reset();
        }

        if (settings->get_showAddTorrentDialog()) {
            emit showAddTorrentDialogs(files, urls, windowActivationToken);
        } else {
            addTorrentLinksWithoutDialog(urls);

            if (const auto existingTorrents = co_await separateTorrentsThatAlreadyExistForFiles(files);
                !existingTorrents.empty()) {
                emit askForMergingTrackers(existingTorrents, windowActivationToken);
                windowActivationToken.reset();
            }
            addTorrentFilesWithoutDialog(files);
        }
    }
}
