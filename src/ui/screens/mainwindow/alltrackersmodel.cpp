// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "alltrackersmodel.h"

#include <map>

#include <QCoreApplication>
#include <QIcon>

#include "rpc/torrent.h"
#include "rpc/tracker.h"
#include "rpc/rpc.h"
#include "ui/itemmodels/modelutils.h"
#include "torrentsproxymodel.h"
#include "desktoputils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    QVariant AllTrackersModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const TrackerItem& item = mTrackers.at(static_cast<size_t>(index.row()));
        switch (role) {
        case TrackerRole:
            return item.tracker;
        case Qt::DecorationRole: {
            if (item.tracker.isEmpty()) {
                return desktoputils::standardDirIcon();
            }
            static const auto networkServerIcon = QIcon::fromTheme("network-server"_L1);
            return networkServerIcon;
        }
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (item.tracker.isEmpty()) {
                //: Filter option of torrents list's tracker filter. %L1 is total number of torrents
                return qApp->translate("tremotesf", "All (%L1)").arg(item.torrents);
            }
            //: Filter option of torrents list's tracker filter. %1 is tracker domain name, %L2 is number of torrents with that tracker
            return qApp->translate("tremotesf", "%1 (%L2)").arg(item.tracker).arg(item.torrents);
        default:
            return {};
        }
    }

    int AllTrackersModel::rowCount(const QModelIndex&) const { return static_cast<int>(mTrackers.size()); }

    QModelIndex AllTrackersModel::indexForTorrentsProxyModelFilter() const {
        if (!torrentsProxyModel()) {
            return {};
        }
        const auto filter = torrentsProxyModel()->trackerFilter();
        for (size_t i = 0, max = mTrackers.size(); i < max; ++i) {
            const auto& item = mTrackers[i];
            if (item.tracker == filter) {
                return index(static_cast<int>(i));
            }
        }
        return {};
    }

    void AllTrackersModel::resetTorrentsProxyModelFilter() const {
        if (torrentsProxyModel()) {
            torrentsProxyModel()->setTrackerFilter({});
        }
    }

    class AllTrackersModelUpdater
        : public ModelListUpdater<AllTrackersModel, AllTrackersModel::TrackerItem, std::map<QString, int>> {
    public:
        inline explicit AllTrackersModelUpdater(AllTrackersModel& model) : ModelListUpdater(model) {}

    protected:
        std::map<QString, int>::iterator
        findNewItemForItem(std::map<QString, int>& newTrackers, const AllTrackersModel::TrackerItem& tracker) override {
            return newTrackers.find(tracker.tracker);
        }

        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        bool updateItem(AllTrackersModel::TrackerItem& tracker, std::pair<const QString, int>&& newTracker) override {
            const auto& [site, torrents] = newTracker;
            if (tracker.torrents != torrents) {
                tracker.torrents = torrents;
                return true;
            }
            return false;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        AllTrackersModel::TrackerItem createItemFromNewItem(std::pair<const QString, int>&& newTracker) override {
            return AllTrackersModel::TrackerItem{.tracker = newTracker.first, .torrents = newTracker.second};
        }
    };

    void AllTrackersModel::update() {
        std::map<QString, int> trackers;
        trackers.emplace(QString(), rpc()->torrentsCount());
        for (const auto& torrent : rpc()->torrents()) {
            for (const Tracker& tracker : torrent->data().trackers) {
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
