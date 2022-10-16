// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
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
#include "tremotesf/rpc/trpc.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QUrl)

SPECIALIZE_FORMATTER_FOR_Q_ENUM(Qt::DropAction)
SPECIALIZE_FORMATTER_FOR_QDEBUG(Qt::DropActions)

template<>
struct fmt::formatter<QDropEvent> : libtremotesf::SimpleFormatter {
    auto format(const QDropEvent& event, format_context& ctx) FORMAT_CONST -> decltype(ctx.out()) {
        const auto mime = event.mimeData();
        return fmt::format_to(ctx.out(), R"(
    proposedAction = {}
    possibleActions = {}
    formats = {}
    urls = {}
    text = {})", event.proposedAction(), event.possibleActions(), mime->formats(), mime->urls(), mime->text());
    }
};

namespace tremotesf {
    namespace {
        constexpr QStringView torrentFileSuffix = u".torrent";
        constexpr QStringView magnetScheme = u"magnet";
        constexpr QStringView magnetQueryPrefixV1 = u"xt=urn:btih:";
        constexpr QStringView magnetQueryPrefixV2 = u"xt=urn:btmh:";
        constexpr QStringView httpScheme = u"http";
        constexpr QStringView httpsScheme = u"https";

        struct DroppedTorrents {
            explicit DroppedTorrents(const QMimeData* mime) {
                // We need to validate whether we are actually opening torrent file (based on extension)
                // or BitTorrent magnet link in order to not accept event and tell OS that we don't want it
                if (mime->hasUrls()) {
                    const auto mimeUrls = mime->urls();
                    for (const auto& url : mimeUrls) {
                        processUrl(url);
                    }
                } else if (mime->hasText()) {
                    const auto text = mime->text();
                    const auto lines = QStringView(text).split(u'\n');
                    for (auto line : lines) {
                        if (!line.isEmpty()) {
                            processUrl(QUrl(line.toString()));
                        }
                    }
                }
            }

            [[nodiscard]] bool isEmpty() const { return files.isEmpty() && urls.isEmpty(); }

            QStringList files{};
            QStringList urls{};

        private:
            void processUrl(const QUrl& url) {
                if (url.isLocalFile()) {
                    if (auto path = url.toLocalFile(); path.endsWith(torrentFileSuffix)) {
                        files.push_back(path);
                    }
                } else {
                    const auto scheme = url.scheme();
                    if (scheme == magnetScheme && url.hasQuery()) {
                        const auto query = url.query();
                        if (query.startsWith(magnetQueryPrefixV1) || query.startsWith(magnetQueryPrefixV2)) {
                            urls.push_back(url.toString());
                        }
                    } else if (scheme == httpScheme || scheme == httpsScheme) {
                        urls.push_back(url.toString());
                    }
                }
            }
        };

        using namespace std::chrono_literals;
        constexpr auto initialDelayedTorrentAddMessageDelay = 500ms;
    }

    MainWindowViewModel::MainWindowViewModel(
        QStringList&& commandLineFiles,
        QStringList&& commandLineUrls,
        Rpc* rpc,
        IpcServer* ipcServer,
        QObject* parent
    ) : QObject(parent),
        mRpc(rpc)
    {
        if (!commandLineFiles.isEmpty() || !commandLineUrls.isEmpty()) {
            QMetaObject::invokeMethod(this, [=] {
                addTorrents(commandLineFiles, commandLineUrls, true);
            }, Qt::QueuedConnection);
        }

        QObject::connect(ipcServer, &IpcServer::windowActivationRequested, this, [=](const auto&, const auto& startupNoficationId) {
            emit showWindow(startupNoficationId);
        });

        QObject::connect(ipcServer, &IpcServer::torrentsAddingRequested, this, [=](const auto& files, const auto& urls) {
            addTorrents(files, urls);
        });

        QObject::connect(rpc, &Rpc::connectedChanged, this, [=] {
            if (rpc->isConnected()) {
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
        logDebug("MainWindowViewModel: event: {}", static_cast<QDropEvent&>(*event));
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
        logDebug("MainWindowViewModel: event: {}", *event);
        const auto dropped = DroppedTorrents(event->mimeData());
        if (dropped.isEmpty()) {
            logWarning("Dropped torrents are empty");
            return;
        }
        logInfo("MainWindowViewModel: accepting QDropEvent");
        event->acceptProposedAction();
        addTorrents(dropped.files, dropped.urls);
    }

    void MainWindowViewModel::pasteShortcutActivated()
    {
        logDebug("MainWindowViewModel: pasteShortcutActivated() called");
        const auto dropped = DroppedTorrents(QGuiApplication::clipboard()->mimeData());
        if (dropped.isEmpty()) {
            logDebug("MainWindowViewModel: ignoring clipboard data");
            return;
        }
        logInfo("MainWindowViewModel: accepting clipboard data");
        addTorrents(dropped.files, dropped.urls);
    }

    void MainWindowViewModel::addTorrents(const QStringList& files, const QStringList& urls, bool showDelayedMessageWithDelay)
    {
        logInfo("MainWindowViewModel: addTorrents() called");
        logInfo("MainWindowViewModel: files = {}", files);
        logInfo("MainWindowViewModel: urls = {}", urls);
        if (mRpc->isConnected()) {
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
                QObject::connect(delayedTorrentAddMessageTimer, &QTimer::timeout, this, [=] {
                    delayedTorrentAddMessageTimer = nullptr;
                    emit showDelayedTorrentAddMessage(files + urls);
                });
                QObject::connect(delayedTorrentAddMessageTimer, &QTimer::timeout, delayedTorrentAddMessageTimer, &QTimer::deleteLater);
                delayedTorrentAddMessageTimer->start();
            } else {
                emit showDelayedTorrentAddMessage(files + urls);
            }
        }
    }
}
