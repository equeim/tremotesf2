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

#ifndef TREMOTESF_PEERSMODEL_H
#define TREMOTESF_PEERSMODEL_H

#include <vector>

#include <QAbstractTableModel>

#include "libtremotesf/peer.h"

namespace libtremotesf
{
    class Torrent;
}

namespace tremotesf
{
    class PeersModel : public QAbstractTableModel
    {
        Q_OBJECT
        Q_PROPERTY(libtremotesf::Torrent* torrent READ torrent WRITE setTorrent NOTIFY torrentChanged)
        Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
    public:
        enum Column
        {
            AddressColumn,
            DownloadSpeedColumn,
            UploadSpeedColumn,
            ProgressBarColumn,
            ProgressColumn,
            FlagsColumn,
            ClientColumn,
            ColumnCount
        };
        static inline constexpr int SortRole = Qt::UserRole;

        explicit PeersModel(libtremotesf::Torrent* torrent = nullptr, QObject* parent = nullptr);
        ~PeersModel() override;

        inline int columnCount(const QModelIndex& = QModelIndex()) const override { return ColumnCount; };
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation, int role) const override;
        int rowCount(const QModelIndex&) const override;
        bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

        libtremotesf::Torrent* torrent() const;
        void setTorrent(libtremotesf::Torrent* torrent);

        bool isLoaded() const;

    private:
        void update(const std::vector<std::pair<int, int>>& removedIndexRanges, const std::vector<std::pair<int, int>>& changedIndexRanges, int addedCount);

        std::vector<libtremotesf::Peer> mPeers;
        libtremotesf::Torrent* mTorrent;
        bool mLoaded;
    signals:
        void torrentChanged();
        void loadedChanged();
    };
}

#endif // TREMOTESF_PEERSMODEL_H
