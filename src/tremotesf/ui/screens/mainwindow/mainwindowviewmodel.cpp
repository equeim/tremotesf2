// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
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

#include "libtremotesf/log.h"
#include "tremotesf/ipc/ipcserver.h"
#include "tremotesf/ui/screens/addtorrent/droppedtorrents.h"
#include "tremotesf/ui/notificationscontroller.h"
#include "tremotesf/settings.h"

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
                [=, this] { addTorrents(commandLineFiles, commandLineUrls, true); },
                Qt::QueuedConnection
            );
        }

        auto ipcServer = IpcServer::createInstance(this);
        QObject::connect(
            ipcServer,
            &IpcServer::windowActivationRequested,
            this,
            [=, this](const auto&, const auto& startupNoficationId) { emit showWindow(startupNoficationId); }
        );

        QObject::connect(
            ipcServer,
            &IpcServer::torrentsAddingRequested,
            this,
            [=, this](const auto& files, const auto& urls) { addTorrents(files, urls); }
        );

        QObject::connect(&mRpc, &Rpc::connectedChanged, this, [this] {
            if (mRpc.isConnected()) {
                if (delayedTorrentAddMessageTimer) {
                    delayedTorrentAddMessageTimer->stop();
                    delayedTorrentAddMessageTimer->deleteLater();
                    delayedTorrentAddMessageTimer = nullptr;
                }
                if ((!mPendingFilesToOpen.empty() || !mPendingUrlsToOpen.empty())) {
                    const QStringList files = std::move(mPendingFilesToOpen);
                    const QStringList urls = std::move(mPendingUrlsToOpen);
                    emit showAddTorrentDialogs(files, urls);
                }
            }
        });
    }

    void MainWindowViewModel::processDragEnterEvent(QDragEnterEvent* event) {
        logDebug("MainWindowViewModel: processing QDragEnterEvent");
        logDebug("MainWindowViewModel: event: {}", formatDropEvent(event));
        const auto dropped = DroppedTorrents(event->mimeData());
        if (dropped.isEmpty()) {
            logDebug("MainWindowViewModel: not accepting QDragEnterEvent");
            return;
        }
        logInfo("MainWindowViewModel: accepting QDragEnterEvent");
        if (event->possibleActions().testFlag(Qt::CopyAction)) {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    }

    void MainWindowViewModel::processDropEvent(QDropEvent* event) {
        logDebug("MainWindowViewModel: processing QDropEvent");
        logDebug("MainWindowViewModel: event: {}", formatDropEvent(event));
        const auto dropped = DroppedTorrents(event->mimeData());
        if (dropped.isEmpty()) {
            logWarning("Dropped torrents are empty");
            return;
        }
        logInfo("MainWindowViewModel: accepting QDropEvent");
        event->acceptProposedAction();
        addTorrents(dropped.files, dropped.urls);
    }

    void MainWindowViewModel::pasteShortcutActivated() {
        logDebug("MainWindowViewModel: pasteShortcutActivated() called");
        const auto dropped = DroppedTorrents(QGuiApplication::clipboard()->mimeData());
        if (dropped.isEmpty()) {
            logDebug("MainWindowViewModel: ignoring clipboard data");
            return;
        }
        logInfo("MainWindowViewModel: accepting clipboard data");
        addTorrents(dropped.files, dropped.urls);
    }

    void MainWindowViewModel::setupNotificationsController(QSystemTrayIcon* trayIcon) {
        const auto controller = NotificationsController::createInstance(trayIcon, this);

        QObject::connect(&mRpc, &Rpc::connectedChanged, this, [controller, this] {
            if (!mRpc.isConnected()) {
                if ((mRpc.error() != Rpc::Error::NoError) && Settings::instance()->notificationOnDisconnecting()) {
                    controller->showNotification(
                        //: Notification title when disconnected from server
                        qApp->translate("tremotesf", "Disconnected"),
                        mRpc.statusString()
                    );
                }
            }
        });

        QObject::connect(&mRpc, &Rpc::addedNotificationRequested, this, [controller](const auto&, const auto& names) {
            controller->showAddedTorrentsNotification(names);
        });

        QObject::connect(
            &mRpc,
            &Rpc::finishedNotificationRequested,
            this,
            [controller](const auto&, const auto& names) { controller->showFinishedTorrentsNotification(names); }
        );

        QObject::connect(controller, &NotificationsController::notificationClicked, this, [this] {
            emit showWindow({});
        });
    }

    void MainWindowViewModel::addTorrents(
        const QStringList& files, const QStringList& urls, bool showDelayedMessageWithDelay
    ) {
        logInfo("MainWindowViewModel: addTorrents() called");
        logInfo("MainWindowViewModel: files = {}", files);
        logInfo("MainWindowViewModel: urls = {}", urls);
        if (mRpc.isConnected()) {
            emit showAddTorrentDialogs(files, urls);
        } else {
            mPendingFilesToOpen.append(files);
            mPendingUrlsToOpen.append(urls);
            logInfo("Delaying opening torrents until connected to server");
            if (showDelayedMessageWithDelay) {
                if (delayedTorrentAddMessageTimer) {
                    delayedTorrentAddMessageTimer->stop();
                    delayedTorrentAddMessageTimer->deleteLater();
                }
                delayedTorrentAddMessageTimer = new QTimer(this);
                delayedTorrentAddMessageTimer->setInterval(initialDelayedTorrentAddMessageDelay);
                delayedTorrentAddMessageTimer->setSingleShot(true);
                QObject::connect(delayedTorrentAddMessageTimer, &QTimer::timeout, this, [=, this] {
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
                emit showDelayedTorrentAddMessage(files + urls);
            }
        }
    }
}
