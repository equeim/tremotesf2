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

#include "trackersmodel.h"

#include <QCoreApplication>

#include "utils.h"

#include "libtremotesf/stdutils.h"
#include "libtremotesf/torrent.h"
#include "libtremotesf/tracker.h"

namespace tremotesf
{
    using libtremotesf::Tracker;

    namespace
    {
        QString trackerStatusString(const Tracker* tracker)
        {
            switch (tracker->status()) {
            case Tracker::Inactive:
                //: Tracker status
                return qApp->translate("tremotesf", "Inactive");
            case Tracker::Active:
                return qApp->translate("tremotesf", "Active", "Tracker status");
            case Tracker::Queued:
                return qApp->translate("tremotesf", "Queued", "Tracker status");
            case Tracker::Updating:
                //: Tracker status
                return qApp->translate("tremotesf", "Updating");
            case Tracker::Error:
            {
                if (tracker->errorMessage().isEmpty()) {
                    return qApp->translate("tremotesf", "Error");
                }
                return qApp->translate("tremotesf", "Error: %1").arg(tracker->errorMessage());
            }
            default:
                return QString();
            }
        }
    }

    TrackersModel::TrackersModel(libtremotesf::Torrent* torrent, QObject* parent)
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
        const Tracker* tracker = mTrackers[index.row()].get();
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case IdRole:
            return tracker->id();
        case AnnounceRole:
            return tracker->announce();
        case StatusStringRole:
            return trackerStatusString(tracker);
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
                return trackerStatusString(tracker);
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

    libtremotesf::Torrent* TrackersModel::torrent() const
    {
        return mTorrent;
    }

    void TrackersModel::setTorrent(libtremotesf::Torrent* torrent)
    {
        if (torrent != mTorrent) {
            mTorrent = torrent;
            if (mTorrent) {
                update();
                QObject::connect(mTorrent, &libtremotesf::Torrent::updated, this, &TrackersModel::update);
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
            ids.append(mTrackers[index.row()]->id());
        }
        return ids;
    }

    Tracker* TrackersModel::trackerAtIndex(const QModelIndex& index) const
    {
        return trackerAtRow(index.row());
    }

    Tracker* TrackersModel::trackerAtRow(int row) const
    {
        return mTrackers[row].get();
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> TrackersModel::roleNames() const
    {
        return {{IdRole, "id"},
                {AnnounceRole, "announce"},
                {StatusStringRole, "statusString"},
                {ErrorRole, "error"},
                {PeersRole, "peers"},
                {NextUpdateRole, "nextUpdate"}};
    }
#endif

    void TrackersModel::update()
    {
        const std::vector<std::shared_ptr<Tracker>>& trackers = mTorrent->trackers();

        for (int i = 0, max = mTrackers.size(); i < max; ++i) {
            if (!contains(trackers, mTrackers[i])) {
                beginRemoveRows(QModelIndex(), i, i);
                mTrackers.erase(mTrackers.begin() + i);
                endRemoveRows();
                i--;
                max--;
            }
        }

        for (const std::shared_ptr<Tracker>& tracker : trackers) {
            if (!contains(mTrackers, tracker)) {
                const int row = mTrackers.size();
                beginInsertRows(QModelIndex(), row, row);
                mTrackers.push_back(tracker);
                endInsertRows();
            }
        }

        emit dataChanged(index(0, 0), index(mTrackers.size() - 1, columnCount() - 1));
    }
}
