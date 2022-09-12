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

#ifndef TREMOTESF_TRACKERSMODEL_H
#define TREMOTESF_TRACKERSMODEL_H

#include <vector>

#include <QAbstractTableModel>

namespace libtremotesf
{
    class Torrent;
    class Tracker;
}

namespace tremotesf
{
    class TrackersModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        enum Column
        {
            AnnounceColumn,
            StatusColumn,
            PeersColumn,
            NextUpdateColumn,
            ColumnCount
        };
        static const int SortRole = Qt::UserRole;

        explicit TrackersModel(libtremotesf::Torrent* torrent = nullptr, QObject* parent = nullptr);

        inline int columnCount(const QModelIndex& = {}) const override { return ColumnCount; }
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;

        libtremotesf::Torrent* torrent() const;
        void setTorrent(libtremotesf::Torrent* torrent);

        QVariantList idsFromIndexes(const QModelIndexList& indexes) const;
        const libtremotesf::Tracker& trackerAtIndex(const QModelIndex& index) const;

    private:
        void update();

        libtremotesf::Torrent* mTorrent;
        std::vector<libtremotesf::Tracker> mTrackers;

        template<typename, typename, typename, typename> friend class ModelListUpdater;
        friend class TrackersModelUpdater;
    };
}

#endif // TREMOTESF_TRACKERSMODEL_H
