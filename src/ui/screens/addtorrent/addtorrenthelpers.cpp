// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "addtorrenthelpers.h"

#include "fileutils.h"
#include "settings.h"
#include "log/log.h"
#include "rpc/rpc.h"
#include "rpc/servers.h"

namespace tremotesf {
    AddTorrentParameters getAddTorrentParameters(Rpc* rpc) {
        auto* const settings = Settings::instance();
        auto* const serverSettings = rpc->serverSettings();
        return {
            .downloadDirectory =
                [&] {
                    const auto lastDir = Servers::instance()->currentServerLastDownloadDirectory(serverSettings);
                    return !lastDir.isEmpty() ? lastDir : serverSettings->data().downloadDirectory;
                }(),
            .priority = settings->lastAddTorrentPriority(),
            .startAfterAdding = settings->lastAddTorrentStartAfterAdding(),
            .deleteTorrentFile = settings->lastAddTorrentDeleteTorrentFile(),
            .moveTorrentFileToTrash = settings->lastAddTorrentMoveTorrentFileToTrash()
        };
    }

    AddTorrentParameters getInitialAddTorrentParameters(Rpc* rpc) {
        auto* const serverSettings = rpc->serverSettings();
        return {
            .downloadDirectory = serverSettings->data().downloadDirectory,
            .priority = TorrentData::Priority::Normal,
            .startAfterAdding = serverSettings->data().startAddedTorrents,
            .deleteTorrentFile = false,
            .moveTorrentFileToTrash = true
        };
    }

    void deleteTorrentFile(const QString& filePath, bool moveToTrash) {
        try {
            if (moveToTrash) {
                try {
                    moveFileToTrash(filePath);
                } catch (const QFileError& e) {
                    warning().logWithException(e, "Failed to move torrent file to trash");
                    deleteFile(filePath);
                }
            } else {
                deleteFile(filePath);
            }
        } catch (const QFileError& e) {
            warning().logWithException(e, "Failed to delete torrent file");
        }
    }
}
