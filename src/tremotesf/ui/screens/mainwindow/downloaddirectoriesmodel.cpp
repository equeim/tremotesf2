// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "downloaddirectoriesmodel.h"

#include <map>

#include <QApplication>
#include <QStyle>

#include "libtremotesf/torrent.h"

#include "tremotesf/ui/itemmodels/modelutils.h"
#include "tremotesf/rpc/trpc.h"
#include "torrentsproxymodel.h"

namespace tremotesf
{
    QVariant DownloadDirectoriesModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return {};
        }
        const DirectoryItem& item = mDirectories[static_cast<size_t>(index.row())];
        switch (role) {
        case DirectoryRole:
            return item.directory;
        case Qt::DecorationRole:
            return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (item.directory.isEmpty()) {
                return qApp->translate("tremotesf", "All (%L1)", "All trackers, %L1 - torrents count").arg(item.torrents);
            }
            //: %1 is a string (directory name or tracker domain name), %L2 is number of torrents
            return qApp->translate("tremotesf", "%1 (%L2)").arg(item.directory).arg(item.torrents);
        default:
            return {};
        }
    }

    int DownloadDirectoriesModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mDirectories.size());
    }

    bool DownloadDirectoriesModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first = mDirectories.begin() + row;
        mDirectories.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    QModelIndex DownloadDirectoriesModel::indexForDirectory(const QString& downloadDirectory) const
    {
        for (size_t i = 0, max = mDirectories.size(); i < max; ++i) {
            const auto& item = mDirectories[i];
            if (item.directory == downloadDirectory) {
                return index(static_cast<int>(i));
            }
        }
        return {};
    }

    QModelIndex DownloadDirectoriesModel::indexForTorrentsProxyModelFilter() const
    {
        if (!torrentsProxyModel()) {
            return {};
        }
        return indexForDirectory(torrentsProxyModel()->downloadDirectoryFilter());
    }

    void DownloadDirectoriesModel::resetTorrentsProxyModelFilter() const
    {
        if (torrentsProxyModel()) {
            torrentsProxyModel()->setDownloadDirectoryFilter({});
        }
    }

    class DownloadDirectoriesModelUpdater : public ModelListUpdater<DownloadDirectoriesModel, DownloadDirectoriesModel::DirectoryItem, std::pair<const QString, int>, std::map<QString, int>> {
    public:
        inline explicit DownloadDirectoriesModelUpdater(DownloadDirectoriesModel& model) : ModelListUpdater(model) {}

    protected:
        std::map<QString, int>::iterator findNewItemForItem(std::map<QString, int>& newItems, const DownloadDirectoriesModel::DirectoryItem& item) override {
            return newItems.find(item.directory);
        }

        bool updateItem(DownloadDirectoriesModel::DirectoryItem& item, std::pair<const QString, int>&& newItem) override {
            const auto& [directory, torrents] = newItem;
            if (item.torrents != torrents) {
                item.torrents = torrents;
                return true;
            }
            return false;
        }

        DownloadDirectoriesModel::DirectoryItem createItemFromNewItem(std::pair<const QString, int>&& newItem) override {
            return DownloadDirectoriesModel::DirectoryItem{newItem.first, newItem.second};
        }
    };

    void DownloadDirectoriesModel::update()
    {
        std::map<QString, int> directories;
        directories.emplace(QString(), rpc()->torrentsCount());
        for (const auto& torrent : rpc()->torrents()) {
            QString directory(torrent->downloadDirectory());
            if (directory.endsWith(QLatin1Char('/'))) {
                directory.chop(1);
            }
            auto found = directories.find(directory);
            if (found == directories.end()) {
                directories.emplace(std::move(directory), 1);
            } else {
                ++(found->second);
            }
        }

        DownloadDirectoriesModelUpdater updater(*this);
        updater.update(mDirectories, std::move(directories));
    }
}
