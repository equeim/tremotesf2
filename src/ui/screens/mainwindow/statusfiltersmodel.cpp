// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "statusfiltersmodel.h"

#include <map>

#include <QCoreApplication>
#include <QIcon>

#include "rpc/rpc.h"
#include "ui/itemmodels/modelutils.h"
#include "desktoputils.h"
#include "torrentsproxymodel.h"

namespace tremotesf {
    QVariant StatusFiltersModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const Item& item = mItems.at(static_cast<size_t>(index.row()));
        switch (role) {
        case FilterRole:
            return item.filter;
        case Qt::DecorationRole:
            switch (item.filter) {
            case TorrentsProxyModel::All: {
                return desktoputils::standardDirIcon();
            }
            case TorrentsProxyModel::Active:
                return desktoputils::statusIcon(desktoputils::ActiveIcon);
            case TorrentsProxyModel::Downloading:
                return desktoputils::statusIcon(desktoputils::DownloadingIcon);
            case TorrentsProxyModel::Seeding:
                return desktoputils::statusIcon(desktoputils::SeedingIcon);
            case TorrentsProxyModel::Paused:
                return desktoputils::statusIcon(desktoputils::PausedIcon);
            case TorrentsProxyModel::Checking:
                return desktoputils::statusIcon(desktoputils::CheckingIcon);
            case TorrentsProxyModel::Errored:
                return desktoputils::statusIcon(desktoputils::ErroredIcon);
            default:
                break;
            }
            break;
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch (item.filter) {
            case TorrentsProxyModel::All:
                //: Filter option of torrents list's status filter. %L1 is total number of torrents
                return qApp->translate("tremotesf", "All (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Active:
                //: Filter option of torrents list's status filter. %L1 is a number of torrents with that status
                return qApp->translate("tremotesf", "Active (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Downloading:
                //: Filter option of torrents list's status filter. %L1 is a number of torrents with that status
                return qApp->translate("tremotesf", "Downloading (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Seeding:
                //: Filter option of torrents list's status filter. %L1 is a number of torrents with that status
                return qApp->translate("tremotesf", "Seeding (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Paused:
                //: Filter option of torrents list's status filter. %L1 is a number of torrents with that status
                return qApp->translate("tremotesf", "Paused (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Checking:
                //: Filter option of torrents list's status filter. %L1 is a number of torrents with that status
                return qApp->translate("tremotesf", "Checking (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Errored:
                //: Filter option of torrents list's status filter. %L1 is a number of torrents with that status
                return qApp->translate("tremotesf", "Errored (%L1)").arg(item.torrents);
            default:
                break;
            }
            break;
        default:
            break;
        }
        return {};
    }

    int StatusFiltersModel::rowCount(const QModelIndex&) const { return static_cast<int>(mItems.size()); }

    QModelIndex StatusFiltersModel::indexForTorrentsProxyModelFilter() const {
        if (!torrentsProxyModel()) {
            return {};
        }
        const auto filter = torrentsProxyModel()->statusFilter();
        for (size_t i = 0, max = mItems.size(); i < max; ++i) {
            const auto& item = mItems[i];
            if (item.filter == filter) {
                return index(static_cast<int>(i));
            }
        }
        return {};
    }

    void StatusFiltersModel::resetTorrentsProxyModelFilter() const {
        if (torrentsProxyModel()) {
            torrentsProxyModel()->setStatusFilter(TorrentsProxyModel::All);
        }
    }

    class StatusFiltersModelUpdater final : public ModelListUpdater<
                                                StatusFiltersModel,
                                                StatusFiltersModel::Item,
                                                std::map<TorrentsProxyModel::StatusFilter, int>> {
    public:
        inline explicit StatusFiltersModelUpdater(StatusFiltersModel& model) : ModelListUpdater(model) {}

    protected:
        std::map<TorrentsProxyModel::StatusFilter, int>::iterator findNewItemForItem(
            std::map<TorrentsProxyModel::StatusFilter, int>& newItems, const StatusFiltersModel::Item& item
        ) override {
            return newItems.find(item.filter);
        }

        bool updateItem(
            StatusFiltersModel::Item& item,
            // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
            std::pair<const TorrentsProxyModel::StatusFilter, int>&& newItem
        ) override {
            const auto& [filter, torrents] = newItem;
            if (item.torrents != torrents) {
                item.torrents = torrents;
                return true;
            }
            return false;
        }

        StatusFiltersModel::Item createItemFromNewItem(
            // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
            std::pair<const TorrentsProxyModel::StatusFilter, int>&& newTracker
        ) override {
            return StatusFiltersModel::Item{.filter = newTracker.first, .torrents = newTracker.second};
        }
    };

    void StatusFiltersModel::update() {
        std::map<TorrentsProxyModel::StatusFilter, int> items{
            {TorrentsProxyModel::All, rpc()->torrentsCount()},
            {TorrentsProxyModel::Active, 0},
            {TorrentsProxyModel::Downloading, 0},
            {TorrentsProxyModel::Seeding, 0},
            {TorrentsProxyModel::Paused, 0},
            {TorrentsProxyModel::Checking, 0},
            {TorrentsProxyModel::Errored, 0}
        };

        const auto processFilter = [&](Torrent* torrent, TorrentsProxyModel::StatusFilter filter) {
            if (TorrentsProxyModel::statusFilterAcceptsTorrent(torrent, filter)) {
                ++(items.find(filter)->second);
            }
        };

        for (const auto& torrent : rpc()->torrents()) {
            processFilter(torrent.get(), TorrentsProxyModel::Active);
            processFilter(torrent.get(), TorrentsProxyModel::Downloading);
            processFilter(torrent.get(), TorrentsProxyModel::Seeding);
            processFilter(torrent.get(), TorrentsProxyModel::Paused);
            processFilter(torrent.get(), TorrentsProxyModel::Checking);
            processFilter(torrent.get(), TorrentsProxyModel::Errored);
        }

        StatusFiltersModelUpdater updater(*this);
        updater.update(mItems, std::move(items));
    }
}
