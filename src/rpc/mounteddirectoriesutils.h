// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_MOUNTEDDIRECTORIESUTILS_H
#define TREMOTESF_RPC_MOUNTEDDIRECTORIESUTILS_H

#include <QString>

namespace tremotesf {
    class Rpc;
    class Torrent;

    bool isServerLocalOrTorrentIsMounted(const Rpc* rpc, const Torrent* torrent);
    QString localTorrentDownloadDirectoryPath(const Rpc* rpc, const Torrent* torrent);
    QString localTorrentRootFilePath(const Rpc* rpc, const Torrent* torrent);
}

#endif // TREMOTESF_RPC_MOUNTEDDIRECTORIESUTILS_H
