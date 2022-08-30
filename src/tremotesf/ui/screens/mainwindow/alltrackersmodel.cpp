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

#include <QApplication>
#include <QStyle>

#include "libtremotesf/torrent.h"
#include "libtremotesf/tracker.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/ui/itemmodels/modelutils.h"
#include "torrentsproxymodel.h"

namespace tremotesf
{
    QVariant AllTrackersModel::data(const QModelIndex& index, int role) const
    {
        const TrackerItem& item = mTrackers[static_cast<size_t>(index.row())];
        switch (role) {
        case TrackerRole:
            return item.tracker;
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
        default:
            return {};
        }
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

    QModelIndex AllTrackersModel::indexForTorrentsProxyModelFilter() const
    {
        if (!torrentsProxyModel()) {
            return {};
        }
        return indexForTracker(torrentsProxyModel()->trackerFilter());
    }

    void AllTrackersModel::resetTorrentsProxyModelFilter() const
    {
        if (torrentsProxyModel()) {
            torrentsProxyModel()->setTrackerFilter({});
        }
    }

    class AllTrackersModelUpdater : public ModelListUpdater<AllTrackersModel, AllTrackersModel::TrackerItem, std::pair<const QString, int>, std::map<QString, int>> {
    public:
        inline explicit AllTrackersModelUpdater(AllTrackersModel& model) : ModelListUpdater(model) {}

    protected:
        std::map<QString, int>::iterator findNewItemForItem(std::map<QString, int>& newTrackers, const AllTrackersModel::TrackerItem& tracker) override {
            return newTrackers.find(tracker.tracker);
        }

        bool updateItem(AllTrackersModel::TrackerItem& tracker, std::pair<const QString, int>&& newTracker) override {
            const auto& [site, torrents] = newTracker;
            if (tracker.torrents != torrents) {
                tracker.torrents = torrents;
                return true;
            }
            return false;
        }

        AllTrackersModel::TrackerItem createItemFromNewItem(std::pair<const QString, int>&& newTracker) override {
            return AllTrackersModel::TrackerItem{newTracker.first, newTracker.second};
        }
    };

    void AllTrackersModel::update()
    {
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

        AllTrackersModelUpdater updater(*this);
        updater.update(mTrackers, std::move(trackers));
    }
}
