/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREMOTESF_RPC_H
#define TREMOTESF_RPC_H

#include "libtremotesf/rpc.h"

namespace tremotesf
{
    class Rpc : public libtremotesf::Rpc
    {
        Q_OBJECT
    public:
        explicit Rpc(QObject* parent = nullptr);
        QString statusString() const;

        bool isIncompleteDirectoryMounted() const;
        bool isTorrentLocalMounted(libtremotesf::Torrent* torrent) const;
        QString localTorrentFilesPath(libtremotesf::Torrent* torrent) const;
        QString localTorrentDownloadDirectoryPath(libtremotesf::Torrent* torrent) const;
    private:
        QString torrentRootFileName(const libtremotesf::Torrent* torrent) const;

        bool mIncompleteDirectoryMounted;
        QString mMountedIncompleteDirectory;

    signals:
        void addedNotificationRequested(const QStringList& hashStrings, const QStringList& names);
        void finishedNotificationRequested(const QStringList& hashStrings, const QStringList& names);
    };
}

#endif // TREMOTESF_RPC_H
