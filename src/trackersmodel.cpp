/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "trackersmodel.h"

#include <QCoreApplication>

#include "torrent.h"
#include "tracker.h"
#include "utils.h"

namespace tremotesf
{
    TrackersModel::TrackersModel(Torrent* torrent, QObject* parent)
        : QAbstractTableModel(parent),
          mTorrent(nullptr)
    {
        setTorrent(torrent);
    }

    int TrackersModel::columnCount(const QModelIndex&) const
    {
#ifdef TREMOTESF_SAILFISHOS
        return 1;
#else
        return ColumnCount;
#endif
    }

    QVariant TrackersModel::data(const QModelIndex& index, int role) const
    {
        const Tracker* tracker = mTrackers.at(index.row()).get();
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case IdRole:
            return tracker->id();
        case AnnounceRole:
            return tracker->announce();
        case StatusStringRole:
            return tracker->statusString();
        case ErrorRole:
            return (tracker->status() == Tracker::Error);
        case PeersRole:
            return tracker->peers();
        case NextUpdateRole:
            return tracker->nextUpdate();
        }
#else
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case AnnounceColumn:
                return tracker->announce();
            case StatusColumn:
                return tracker->statusString();
            case PeersColumn:
                return tracker->peers();
            case NextUpdateColumn:
                if (tracker->nextUpdate() != -1) {
                    return Utils::formatEta(tracker->nextUpdate());
                }
                break;
            }
        } else if (role == SortRole) {
            if (index.column() == NextUpdateColumn) {
                return tracker->nextUpdate();
            }
            return data(index, Qt::DisplayRole);
        }
#endif
        return QVariant();
    }

#ifndef TREMOTESF_SAILFISHOS
    QVariant TrackersModel::headerData(int section, Qt::Orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case AnnounceColumn:
                return qApp->translate("tremotesf", "Address");
            case StatusColumn:
                return qApp->translate("tremotesf", "Status");
            case NextUpdateColumn:
                return qApp->translate("tremotesf", "Next Update");
            case PeersColumn:
                return qApp->translate("tremotesf", "Peers");
            }
        }
        return QVariant();
    }
#endif

    int TrackersModel::rowCount(const QModelIndex&) const
    {
        return mTrackers.size();
    }

    Torrent* TrackersModel::torrent() const
    {
        return mTorrent;
    }

    void TrackersModel::setTorrent(Torrent* torrent)
    {
        if (torrent != mTorrent) {
            mTorrent = torrent;
            if (mTorrent) {
                update();
                QObject::connect(mTorrent, &Torrent::updated, this, &TrackersModel::update);
            } else {
                beginResetModel();
                mTrackers.clear();
                endResetModel();
            }
        }
    }

    QVariantList TrackersModel::idsFromIndexes(const QModelIndexList& indexes) const
    {
        QVariantList ids;
        for (const QModelIndex& index : indexes) {
            ids.append(mTrackers.at(index.row())->id());
        }
        return ids;
    }

    Tracker* TrackersModel::trackerAtIndex(const QModelIndex& index) const
    {
        return trackerAtRow(index.row());
    }

    Tracker* TrackersModel::trackerAtRow(int row) const
    {
        return mTrackers.at(row).get();
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> TrackersModel::roleNames() const
    {
        return {{IdRole, QByteArrayLiteral("id")},
                {AnnounceRole, QByteArrayLiteral("announce")},
                {StatusStringRole, QByteArrayLiteral("statusString")},
                {ErrorRole, QByteArrayLiteral("error")},
                {PeersRole, QByteArrayLiteral("peers")},
                {NextUpdateRole, QByteArrayLiteral("nextUpdate")}};
    }
#endif

    void TrackersModel::update()
    {
        const QList<std::shared_ptr<Tracker>>& trackers = mTorrent->trackers();

        for (int i = 0, max = mTrackers.size(); i < max; ++i) {
            if (!trackers.contains(mTrackers.at(i))) {
                beginRemoveRows(QModelIndex(), i, i);
                mTrackers.removeAt(i);
                endRemoveRows();
                i--;
                max--;
            }
        }

        for (const std::shared_ptr<Tracker>& tracker : trackers) {
            if (!mTrackers.contains(tracker)) {
                const int row = mTrackers.size();
                beginInsertRows(QModelIndex(), row, row);
                mTrackers.append(tracker);
                endInsertRows();
            }
        }

        emit dataChanged(index(0, 0), index(mTrackers.size() - 1, columnCount() - 1));
    }
}
