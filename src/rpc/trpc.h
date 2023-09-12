// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_H
#define TREMOTESF_RPC_H

#include "rpc.h"

namespace tremotesf {
    class Rpc final : public BaseRpc {
        Q_OBJECT

    public:
        explicit Rpc(QObject* parent = nullptr);
        QString statusString() const;

        bool isIncompleteDirectoryMounted() const;
        bool isTorrentLocalMounted(Torrent* torrent) const;
        QString localTorrentFilesPath(Torrent* torrent) const;
        QString localTorrentDownloadDirectoryPath(Torrent* torrent) const;

    private:
        QString torrentRootFileName(const Torrent* torrent) const;

        bool mIncompleteDirectoryMounted;
        QString mMountedIncompleteDirectory;

    signals:
        void addedNotificationRequested(const QStringList& hashStrings, const QStringList& names);
        void finishedNotificationRequested(const QStringList& hashStrings, const QStringList& names);
    };
}

#endif // TREMOTESF_RPC_H
