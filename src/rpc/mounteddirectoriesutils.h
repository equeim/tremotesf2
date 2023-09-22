// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_MOUNTEDDIRECTORIESUTILS_H
#define TREMOTESF_RPC_MOUNTEDDIRECTORIESUTILS_H

#include <QString>

namespace tremotesf {
    class Rpc;
    class Torrent;

    bool isIncompleteDirectoryMounted(const Rpc* rpc);
    bool isTorrentLocalMounted(const Rpc* rpc, const Torrent* torrent);
    QString localTorrentDownloadDirectoryPath(const Rpc* rpc, const Torrent* torrent);
    QString localTorrentFilesPath(const Rpc* rpc, const Torrent* torrent);
}

#endif // TREMOTESF_RPC_MOUNTEDDIRECTORIESUTILS_H
