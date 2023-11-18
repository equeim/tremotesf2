// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mounteddirectoriesutils.h"

#include "rpc.h"
#include "serversettings.h"
#include "servers.h"

#include <QFileInfo>
#include <QStringBuilder>

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
        if (torrent->data().singleFile && torrent->data().leftUntilDone > 0 &&
            rpc->serverSettings()->data().renameIncompleteFiles) {
            return downloadDirectoryPath % '/' % torrentName % ".part"_l1;
        }
        return downloadDirectoryPath % '/' % torrentName;
    }
}
