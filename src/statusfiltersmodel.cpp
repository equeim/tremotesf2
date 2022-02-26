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

#include "statusfiltersmodel.h"

#include <map>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#include "desktop/desktoputils.h"
#else
#include <QCoreApplication>
#endif

#include "modelutils.h"
#include "torrentsproxymodel.h"
#include "trpc.h"

namespace tremotesf
{
    QVariant StatusFiltersModel::data(const QModelIndex& index, int role) const
    {
        const Item& item = mItems[static_cast<size_t>(index.row())];
        switch (role) {
        case FilterRole:
            return item.filter;
#ifdef TREMOTESF_SAILFISHOS
        case TorrentsRole:
            return item.torrents;
        case FilterNameRole:
            switch (item.filter) {
            case TorrentsProxyModel::All:
                return qApp->translate("tremotesf", "All", "All torrents");
            case TorrentsProxyModel::Active:
                return qApp->translate("tremotesf", "Active", "Active torrents");
            case TorrentsProxyModel::Downloading:
                return qApp->translate("tremotesf", "Downloading", "Downloading torrents");
            case TorrentsProxyModel::Seeding:
                return qApp->translate("tremotesf", "Seeding", "Seeding torrents");
            case TorrentsProxyModel::Paused:
                return qApp->translate("tremotesf", "Paused", "Paused torrents");
            case TorrentsProxyModel::Checking:
                return qApp->translate("tremotesf", "Checking", "Checking torrents");
            case TorrentsProxyModel::Errored:
                return qApp->translate("tremotesf", "Errored", "Errored torrents");
            default:
                break;
            }
#else
        case Qt::DecorationRole:
            switch (item.filter) {
            case TorrentsProxyModel::All:
                return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            case TorrentsProxyModel::Active:
                return QIcon(desktoputils::statusIconPath(desktoputils::ActiveIcon));
            case TorrentsProxyModel::Downloading:
                return QIcon(desktoputils::statusIconPath(desktoputils::DownloadingIcon));
            case TorrentsProxyModel::Seeding:
                return QIcon(desktoputils::statusIconPath(desktoputils::SeedingIcon));
            case TorrentsProxyModel::Paused:
                return QIcon(desktoputils::statusIconPath(desktoputils::PausedIcon));
            case TorrentsProxyModel::Checking:
                return QIcon(desktoputils::statusIconPath(desktoputils::CheckingIcon));
            case TorrentsProxyModel::Errored:
                return QIcon(desktoputils::statusIconPath(desktoputils::ErroredIcon));
            default:
                break;
            }
            break;
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            switch (item.filter) {
            case TorrentsProxyModel::All:
                return qApp->translate("tremotesf", "All (%L1)", "All torrents, %L1 - torrents count").arg(item.torrents);
            case TorrentsProxyModel::Active:
                return qApp->translate("tremotesf", "Active (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Downloading:
                return qApp->translate("tremotesf", "Downloading (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Seeding:
                return qApp->translate("tremotesf", "Seeding (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Paused:
                return qApp->translate("tremotesf", "Paused (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Checking:
                return qApp->translate("tremotesf", "Checking (%L1)").arg(item.torrents);
            case TorrentsProxyModel::Errored:
                return qApp->translate("tremotesf", "Errored (%L1)").arg(item.torrents);
            default:
                break;
            }
            break;
#endif
        }
        return {};
    }

    int StatusFiltersModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mItems.size());
    }

    bool StatusFiltersModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first = mItems.begin() + row;
        mItems.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    QModelIndex StatusFiltersModel::indexForStatusFilter(TorrentsProxyModel::StatusFilter filter) const
    {
        for (size_t i = 0, max = mItems.size(); i < max; ++i) {
            const auto& item = mItems[i];
            if (item.filter == filter) {
                return index(static_cast<int>(i));
            }
        }
        return {};
    }

    QModelIndex StatusFiltersModel::indexForTorrentsProxyModelFilter() const
    {
        if (!torrentsProxyModel()) {
            return {};
        }
        return indexForStatusFilter(torrentsProxyModel()->statusFilter());
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> StatusFiltersModel::roleNames() const
    {
        return {{FilterRole, "filter"},
                {FilterNameRole, "filterName"},
                {TorrentsRole, "torrents"}};
    }
#endif

    void StatusFiltersModel::resetTorrentsProxyModelFilter() const
    {
        if (torrentsProxyModel()) {
            torrentsProxyModel()->setStatusFilter(TorrentsProxyModel::All);
        }
    }

    class StatusFiltersModelUpdater : public ModelListUpdater<StatusFiltersModel, StatusFiltersModel::Item, std::pair<const TorrentsProxyModel::StatusFilter, int>, std::map<TorrentsProxyModel::StatusFilter, int>> {
    public:
        inline explicit StatusFiltersModelUpdater(StatusFiltersModel& model) : ModelListUpdater(model) {}

    protected:
        std::map<TorrentsProxyModel::StatusFilter, int>::iterator findNewItemForItem(std::map<TorrentsProxyModel::StatusFilter, int>& newItems, const StatusFiltersModel::Item& item) override {
            return newItems.find(item.filter);
        }

        bool updateItem(StatusFiltersModel::Item& item, std::pair<const TorrentsProxyModel::StatusFilter, int>&& newItem) override {
            const auto& [filter, torrents] = newItem;
            if (item.torrents != torrents) {
                item.torrents = torrents;
                return true;
            }
            return false;
        }

        StatusFiltersModel::Item createItemFromNewItem(std::pair<const TorrentsProxyModel::StatusFilter, int>&& newTracker) override {
            return StatusFiltersModel::Item{newTracker.first, newTracker.second};
        }
    };

    void StatusFiltersModel::update()
    {
        std::map<TorrentsProxyModel::StatusFilter, int> items{
            {TorrentsProxyModel::All, rpc()->torrentsCount()},
            {TorrentsProxyModel::Active, 0},
            {TorrentsProxyModel::Downloading, 0},
            {TorrentsProxyModel::Seeding, 0},
            {TorrentsProxyModel::Paused, 0},
            {TorrentsProxyModel::Checking, 0},
            {TorrentsProxyModel::Errored, 0}
        };

        const auto processFilter = [&](libtremotesf::Torrent* torrent, TorrentsProxyModel::StatusFilter filter) {
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
