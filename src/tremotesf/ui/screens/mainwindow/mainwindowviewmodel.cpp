// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindowviewmodel.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
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
        constexpr QStringView magnetScheme = u"magnet";
        constexpr QStringView magnetQueryPrefixV1 = u"xt=urn:btih:";
        constexpr QStringView magnetQueryPrefixV2 = u"xt=urn:btmh:";
        constexpr QStringView torrentFileSuffix = u".torrent";

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

            bool isEmpty() const { return files.isEmpty() && urls.isEmpty(); }

            QStringList files{};
            QStringList urls{};

        private:
            void processUrl(const QUrl& url) {
                if (url.isLocalFile()) {
                    if (auto path = url.toLocalFile(); path.endsWith(torrentFileSuffix)) {
                        files.push_back(std::move(path));
                    }
                } else if (url.scheme() == magnetScheme && url.hasQuery()) {
                    const auto query = url.query();
                    if (query.startsWith(magnetQueryPrefixV1) || query.startsWith(magnetQueryPrefixV2)) {
                        urls.push_back(url.toString());
                    }
                }
            }
        };
    }

    MainWindowViewModel::MainWindowViewModel(
        QStringList&& commandLineFiles,
        QStringList&& commandLineUrls,
        Rpc* rpc,
        IpcServer* ipcServer,
        QObject* parent
    ) : QObject(parent),
        mRpc(rpc),
        mPendingFilesToOpen(std::move(commandLineFiles)),
        mPendingUrlsToOpen(std::move(commandLineUrls))
    {
        QObject::connect(ipcServer, &IpcServer::windowActivationRequested, this, [=](const auto&, const auto& startupNoficationId) {
            emit showWindow(startupNoficationId);
        });

        QObject::connect(ipcServer, &IpcServer::torrentsAddingRequested, this, [=](const auto& files, const auto& urls) {
            if (rpc->isConnected()) {
                emit showAddTorrentDialogs(files, urls);
            } else {
                logInfo("Delaying opening torrents until connected to server");
                mPendingFilesToOpen.append(files);
                mPendingUrlsToOpen.append(urls);
            }
        });

        QObject::connect(rpc, &Rpc::connectedChanged, this, [=] {
            if (rpc->isConnected() && (!mPendingFilesToOpen.empty() || !mPendingUrlsToOpen.empty())) {
                const QStringList files = std::move(mPendingFilesToOpen);
                const QStringList urls = std::move(mPendingUrlsToOpen);
                emit showAddTorrentDialogs(files, urls);
            }
        });
    }

    void MainWindowViewModel::processDragEnterEvent(QDragEnterEvent* event) {
        logDebug("MainWindowViewModel: processing QDragEnterEvent");
        logDebug("MainWindowViewModel: event: {}", static_cast<QDropEvent&>(*event));
        const auto dropped = DroppedTorrents(event->mimeData());
        if (!dropped.isEmpty()) {
            logInfo("MainWindowViewModel: accepting QDragEnterEvent");
            if (event->possibleActions().testFlag(Qt::CopyAction)) {
                event->setDropAction(Qt::CopyAction);
                event->accept();
            } else {
                event->acceptProposedAction();
            }
        } else {
            logDebug("MainWindowViewModel: not accepting QDragEnterEvent");
        }
    }

    void MainWindowViewModel::processDropEvent(QDropEvent* event) {
        logDebug("MainWindowViewModel: processing QDropEvent");
        logDebug("MainWindowViewModel: event: {}", *event);
        const auto dropped = DroppedTorrents(event->mimeData());
        if (dropped.isEmpty()) {
            logWarning("Dropped torrents are empty");
        } else {
            logInfo("MainWindowViewModel: accepting QDropEvent");
            logInfo("MainWindowViewModel: files = {}", dropped.files);
            logInfo("MainWindowViewModel: urls = {}", dropped.urls);
            if (mRpc->isConnected()) {
                emit showAddTorrentDialogs(dropped.files, dropped.urls);
            } else {
                logInfo("Delaying opening torrents until connected to server");
                mPendingFilesToOpen.append(dropped.files);
                mPendingUrlsToOpen.append(dropped.urls);
            }
            event->acceptProposedAction();
        }
    }
}
