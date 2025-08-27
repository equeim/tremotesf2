// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mounteddirectoriesutils.h"

#include "rpc.h"
#include "serversettings.h"
#include "servers.h"

#include <QFileInfo>
#include <QStringBuilder>

using namespace Qt::StringLiterals;

namespace tremotesf {
    bool isServerLocalOrTorrentIsMounted(const Rpc* rpc, const Torrent* torrent) {
        if (rpc->isLocal()) {
            return true;
        }
        if (!Servers::instance()->currentServerHasMountedDirectories()) {
            return false;
        }
        return !localTorrentDownloadDirectoryPath(rpc, torrent).isEmpty();
    }

    QString localTorrentDownloadDirectoryPath(const Rpc* rpc, const Torrent* torrent) {
        const auto serverSettings = rpc->serverSettings();
        const bool incompleteDirectoryEnabled = serverSettings->data().incompleteDirectoryEnabled;
        QString directory{};
        if (incompleteDirectoryEnabled && torrent->data().leftUntilDone > 0) {
            directory = serverSettings->data().incompleteDirectory;
        } else {
            directory = torrent->data().downloadDirectory;
        }
        if (!rpc->isLocal() && !directory.isEmpty() && Servers::instance()->currentServerHasMountedDirectories()) {
            directory = Servers::instance()->fromRemoteToLocalDirectory(directory, serverSettings);
        }
        return directory;
    }

    QString localTorrentRootFilePath(const Rpc* rpc, const Torrent* torrent) {
        const QString downloadDirectoryPath = localTorrentDownloadDirectoryPath(rpc, torrent);
        if (downloadDirectoryPath.isEmpty()) {
            return {};
        }
        const auto& torrentName = torrent->data().name;
        const QString rootFilePath = downloadDirectoryPath % '/' % torrentName;

        if (rpc->serverSettings()->data().renameIncompleteFiles && torrent->data().leftUntilDone > 0) {
            const QString incompleteRootFilePath = rootFilePath % ".part"_L1;
            // Don't return incomplete file path unless it exists because we don't know whether it's supposed to be a directory or a file,
            // and in case of directory this path can't exist so we don't want to try to open it
            if (QFileInfo::exists(incompleteRootFilePath)) {
                return incompleteRootFilePath;
            }
        }
        return rootFilePath;
    }
}
