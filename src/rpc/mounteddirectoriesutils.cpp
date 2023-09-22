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
    namespace {
        QString getMountedIncompleteDirectory(const ServerSettings* serverSettings) {
            return Servers::instance()->fromRemoteToLocalDirectory(
                serverSettings->data().incompleteDirectory,
                serverSettings
            );
        }

        QString getTorrentRootFileName(const ServerSettings* serverSettings, const Torrent* torrent) {
            if (torrent->data().singleFile && torrent->data().leftUntilDone > 0 &&
                serverSettings->data().renameIncompleteFiles) {
                return torrent->data().name % ".part"_l1;
            }
            return torrent->data().name;
        }
    }

    bool isIncompleteDirectoryMounted(const Rpc* rpc) {
        return !getMountedIncompleteDirectory(rpc->serverSettings()).isEmpty();
    }

    bool isTorrentLocalMounted(const Rpc* rpc, const Torrent* torrent) {
        if (rpc->isLocal()) {
            return true;
        }
        if (!Servers::instance()->currentServerHasMountedDirectories()) {
            return false;
        }
        const auto serverSettings = rpc->serverSettings();
        if (serverSettings->data().incompleteDirectoryEnabled && torrent->data().leftUntilDone > 0 &&
            !isIncompleteDirectoryMounted(rpc)) {
            return false;
        }
        const auto mountedDirectory =
            Servers::instance()->fromRemoteToLocalDirectory(torrent->data().downloadDirectory, serverSettings);
        return !mountedDirectory.isEmpty();
    }

    QString localTorrentDownloadDirectoryPath(const Rpc* rpc, const Torrent* torrent) {
        const auto serverSettings = rpc->serverSettings();
        const bool incompleteDirectoryEnabled = serverSettings->data().incompleteDirectoryEnabled;
        QString filePath;
        if (rpc->isLocal()) {
            if (incompleteDirectoryEnabled && torrent->data().leftUntilDone > 0 &&
                QFileInfo::exists(
                    serverSettings->data().incompleteDirectory % '/' % getTorrentRootFileName(serverSettings, torrent)
                )) {
                filePath = serverSettings->data().incompleteDirectory;
            } else {
                filePath = torrent->data().downloadDirectory;
            }
        } else {
            if (Servers::instance()->currentServerHasMountedDirectories()) {
                const auto mountedIncompleteDirectory = getMountedIncompleteDirectory(serverSettings);
                if (incompleteDirectoryEnabled && torrent->data().leftUntilDone > 0 &&
                    !mountedIncompleteDirectory.isEmpty() &&
                    QFileInfo::exists(
                        mountedIncompleteDirectory % '/' % getTorrentRootFileName(serverSettings, torrent)
                    )) {
                    filePath = mountedIncompleteDirectory;
                } else {
                    filePath = Servers::instance()->fromRemoteToLocalDirectory(
                        torrent->data().downloadDirectory,
                        serverSettings
                    );
                }
            }
        }
        return filePath;
    }

    QString localTorrentFilesPath(const Rpc* rpc, const Torrent* torrent) {
        const QString downloadDirectoryPath(localTorrentDownloadDirectoryPath(rpc, torrent));
        if (downloadDirectoryPath.isEmpty()) {
            return {};
        }
        return downloadDirectoryPath % '/' % getTorrentRootFileName(rpc->serverSettings(), torrent);
    }
}
