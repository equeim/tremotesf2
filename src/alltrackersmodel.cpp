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

#include "alltrackersmodel.h"

#include <map>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "torrentsproxymodel.h"

#include "libtremotesf/qtsupport.h"
#include "libtremotesf/torrent.h"
#include "libtremotesf/tracker.h"
#include "modelutils.h"
#include "trpc.h"

namespace tremotesf
{
#ifdef TREMOTESF_SAILFISHOS
    namespace
    {
        enum Role
        {
            TrackerRole = Qt::UserRole,
            TorrentsRole
        };
    }
#endif

    QVariant AllTrackersModel::data(const QModelIndex& index, int role) const
    {
        const TrackerItem& item = mTrackers[static_cast<size_t>(index.row())];
        switch (role) {
        case TrackerRole:
            return item.tracker;
#ifdef TREMOTESF_SAILFISHOS
        case TorrentsRole:
            return item.torrents;
        }
#else
        case Qt::DecorationRole:
            if (item.tracker.isEmpty()) {
                return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            }
            return QIcon::fromTheme(QLatin1String("network-server"));
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (item.tracker.isEmpty()) {
                return qApp->translate("tremotesf", "All (%L1)", "All trackers, %L1 - torrents count").arg(item.torrents);
            }
            //: %1 is a string (directory name or tracker domain name), %L2 is number of torrents
            return qApp->translate("tremotesf", "%1 (%L2)").arg(item.tracker).arg(item.torrents);
        }
#endif
        return {};
    }

    int AllTrackersModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mTrackers.size());
    }

    bool AllTrackersModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first = mTrackers.begin() + row;
        mTrackers.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    QModelIndex AllTrackersModel::indexForTracker(const QString& tracker) const
    {
        for (size_t i = 0, max = mTrackers.size(); i < max; ++i) {
            const auto& item = mTrackers[i];
            if (item.tracker == tracker) {
                return index(static_cast<int>(i));
            }
        }
        return {};
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> AllTrackersModel::roleNames() const
    {
        return {{TrackerRole, "tracker"},
                {TorrentsRole, "torrents"}};
    }
#endif

    void AllTrackersModel::update()
    {
        if (rpc()->torrents().empty()) {
            if (!mTrackers.empty()) {
                mTrackers[0].torrents = 0;
                const auto firstIndex = index(0);
                emit dataChanged(firstIndex, firstIndex);

                beginRemoveRows({}, 1, static_cast<int>(mTrackers.size() - 1));
                mTrackers.erase(mTrackers.begin() + 1, mTrackers.end());
                endRemoveRows();
            }
            return;
        }

        std::map<QString, int> trackers;
        trackers.emplace(QString(), rpc()->torrentsCount());
        for (const auto& torrent : rpc()->torrents()) {
            for (const libtremotesf::Tracker& tracker : torrent->trackers()) {
                const QString& site = tracker.site();
                auto found = trackers.find(site);
                if (found == trackers.end()) {
                    trackers.emplace(site, 1);
                } else {
                    ++(found->second);
                }
            }
        }
        const auto trackersEnd = trackers.end();

        if (!mTrackers.empty()) {
            ModelBatchRemover modelRemover(this);
            size_t i = (mTrackers.size() - 1);
            while (i != 0) {
                const auto found = trackers.find(mTrackers[i].tracker);
                if (found == trackersEnd) {
                    modelRemover.remove(static_cast<int>(i));
                }
                --i;
            }
            modelRemover.remove();
        }

        ModelBatchChanger changer(this);
        for (size_t i = 0, max = mTrackers.size(); i < max; ++i) {
            TrackerItem& item = mTrackers[i];
            const auto found = trackers.find(item.tracker);
            if (found != trackersEnd) {
                const int torrents = found->second;
                if (torrents != item.torrents) {
                    item.torrents = torrents;
                    changer.changed(static_cast<int>(i));
                }
                trackers.erase(found);
            }
        }
        changer.changed();

        if (!trackers.empty()) {
            const int firstRow = static_cast<int>(mTrackers.size());
            beginInsertRows({}, firstRow, firstRow + static_cast<int>(trackers.size()) - 1);
            mTrackers.reserve(mTrackers.size() + trackers.size());
            for (const auto& i : trackers) {
                const QString& tracker = i.first;
                const int torrents = i.second;
                mTrackers.push_back(TrackerItem{tracker, torrents});
            }
            endInsertRows();
        }
    }
}
