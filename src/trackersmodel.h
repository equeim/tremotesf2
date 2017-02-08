/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include <memory>

#include <QAbstractTableModel>
#include <QList>

namespace tremotesf
{
    class Torrent;
    class Tracker;

    class TrackersModel : public QAbstractTableModel
    {
        Q_OBJECT
        Q_PROPERTY(tremotesf::Torrent* torrent READ torrent WRITE setTorrent)
    public:
#ifdef TREMOTESF_SAILFISHOS
        enum Role
        {
            IdRole,
            AnnounceRole,
            StatusStringRole,
            ErrorRole,
            PeersRole,
            NextUpdateRole
        };
        Q_ENUMS(Role)
#else
        enum Column
        {
            AnnounceColumn,
            StatusColumn,
            PeersColumn,
            NextUpdateColumn,
            ColumnCount
        };
        static const int SortRole = Qt::UserRole;
#endif

        explicit TrackersModel(Torrent* torrent = nullptr, QObject* parent = nullptr);

        int columnCount(const QModelIndex& = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
#ifndef TREMOTESF_SAILFISHOS
        QVariant headerData(int section, Qt::Orientation, int role) const override;
#endif
        int rowCount(const QModelIndex&) const override;

        Torrent* torrent() const;
        void setTorrent(Torrent* torrent);

        Q_INVOKABLE QVariantList idsFromIndexes(const QModelIndexList& indexes) const;
        Tracker* trackerAtIndex(const QModelIndex& index) const;
        Tracker* trackerAtRow(int row) const;

#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif

    private:
        void update();

        Torrent* mTorrent;
        QList<std::shared_ptr<Tracker>> mTrackers;
    };
}

#endif // TREMOTESF_TRACKERSMODEL_H
