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
#include <QTimer>
#include <QUrl>

#include <fmt/ranges.h>

#include "log/log.h"
#include "ipc/ipcserver.h"
#include "rpc/servers.h"
#include "ui/screens/addtorrent/addtorrenthelpers.h"
#include "ui/screens/addtorrent/droppedtorrents.h"
#include "ui/notificationscontroller.h"
#include "settings.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

SPECIALIZE_FORMATTER_FOR_Q_ENUM(Qt::DropAction)
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
                [commandLineFiles = std::move(commandLineFiles), commandLineUrls = std::move(commandLineUrls), this] {
                    addTorrents(commandLineFiles, commandLineUrls, {});
                },
                Qt::QueuedConnection
            );
        }

        const auto* const ipcServer = IpcServer::createInstance(this);
        QObject::connect(
            ipcServer,
            &IpcServer::windowActivationRequested,
            this,
            [=, this](const auto&, const auto& activationToken) { emit showWindow(activationToken); }
        );

        QObject::connect(
            ipcServer,
            &IpcServer::torrentsAddingRequested,
            this,
            [=, this](const auto& files, const auto& urls, const auto& activationToken) {
                addTorrents(files, urls, activationToken);
            }
        );

        QObject::connect(&mRpc, &Rpc::connectedChanged, this, [this] {
            if (mRpc.isConnected()) {
                if (delayedTorrentAddMessageTimer) {
                    info().log("Cancelling showing delayed torrent adding message");
                    delayedTorrentAddMessageTimer->stop();
                    delayedTorrentAddMessageTimer->deleteLater();
                    delayedTorrentAddMessageTimer = nullptr;
                }
                if ((!mPendingFilesToOpen.empty() || !mPendingUrlsToOpen.empty())) {
                    const QStringList files = std::move(mPendingFilesToOpen);
                    const QStringList urls = std::move(mPendingUrlsToOpen);
                    emit showAddTorrentDialogs(files, urls, {});
                }
            }
        });

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
        const auto dropped = DroppedTorrents(event->mimeData());
        if (dropped.isEmpty()) {
            warning().log("Dropped torrents are empty");
            return;
        }
        info().log("MainWindowViewModel: accepting QDropEvent");
        event->acceptProposedAction();
        addTorrents(dropped.files, dropped.urls);
    }

    void MainWindowViewModel::pasteShortcutActivated() {
        debug().log("MainWindowViewModel: pasteShortcutActivated() called");
        const auto dropped = DroppedTorrents(QGuiApplication::clipboard()->mimeData());
        if (dropped.isEmpty()) {
            debug().log("MainWindowViewModel: ignoring clipboard data");
            return;
        }
        info().log("MainWindowViewModel: accepting clipboard data");
        addTorrents(dropped.files, dropped.urls);
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
        if (Settings::instance()->connectOnStartup()) {
            mRpc.connect();
        }
        return StartupActionResult::DoNothing;
    }

    void MainWindowViewModel::addTorrentFilesWithoutDialog(const QStringList& files) {
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
                parameters.startAfterAdding
            );
            if (parameters.deleteTorrentFile) {
                deleteTorrentFile(filePath, parameters.moveTorrentFileToTrash);
            }
        }
    }

    void MainWindowViewModel::addTorrentLinksWithoutDialog(const QStringList& urls) {
        const auto parameters = getAddTorrentParameters(&mRpc);
        for (const auto& url : urls) {
            mRpc.addTorrentLink(url, parameters.downloadDirectory, parameters.priority, parameters.startAfterAdding);
        }
    }

    void MainWindowViewModel::addTorrents(
        const QStringList& files, const QStringList& urls, const std::optional<QByteArray>& windowActivationToken
    ) {
        info().log("MainWindowViewModel: addTorrents() called");
        info().log("MainWindowViewModel: files = {}", files);
        info().log("MainWindowViewModel: urls = {}", urls);
        const auto connectionState = mRpc.connectionState();
        if (connectionState == RpcConnectionState::Connected) {
            emit showAddTorrentDialogs(files, urls, windowActivationToken);
        } else {
            mPendingFilesToOpen.append(files);
            mPendingUrlsToOpen.append(urls);
            info().log("Postponing opening torrents until connected to server");
            emit showWindow(windowActivationToken);
            // If we are connecting then wait a bit before showing message
            if (connectionState == RpcConnectionState::Connecting) {
                info().log("We are already connecting, wait a bit before showing message");
                if (delayedTorrentAddMessageTimer) {
                    delayedTorrentAddMessageTimer->stop();
                    delayedTorrentAddMessageTimer->deleteLater();
                }
                delayedTorrentAddMessageTimer = new QTimer(this);
                delayedTorrentAddMessageTimer->setInterval(initialDelayedTorrentAddMessageDelay);
                delayedTorrentAddMessageTimer->setSingleShot(true);
                QObject::connect(delayedTorrentAddMessageTimer, &QTimer::timeout, this, [=, this] {
                    info().log("Showing delayed torrent adding message");
                    delayedTorrentAddMessageTimer = nullptr;
                    emit showDelayedTorrentAddMessage(files + urls);
                });
                QObject::connect(
                    delayedTorrentAddMessageTimer,
                    &QTimer::timeout,
                    delayedTorrentAddMessageTimer,
                    &QTimer::deleteLater
                );
                delayedTorrentAddMessageTimer->start();
            } else {
                info().log("Showing delayed torrent adding message");
                emit showDelayedTorrentAddMessage(files + urls);
            }
        }
    }
}
