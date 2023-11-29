// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "downloaddirectoriesmodel.h"

#include <algorithm>
#include <map>

#include <QApplication>
#include <QStyle>

#include "rpc/pathutils.h"
#include "rpc/torrent.h"
#include "ui/itemmodels/modelutils.h"
#include "rpc/rpc.h"
#include "torrentsproxymodel.h"
#include "ui/screens/mainwindow/downloaddirectoriesmodel.h"

namespace tremotesf {
    QVariant DownloadDirectoriesModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const DirectoryItem& item = mDirectories.at(static_cast<size_t>(index.row()));
        switch (role) {
        case DirectoryRole:
            return item.directory;
        case Qt::DecorationRole:
            return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (item.directory.isEmpty()) {
                //: Filter option of torrents list's download directory filter. %L1 is total number of torrents
                return qApp->translate("tremotesf", "All (%L1)").arg(item.torrents);
            }
            //: Filter option of torrents list's download directory filter. %1 is download directory, %L2 is number of torrents with that download directory
            return qApp->translate("tremotesf", "%1 (%L2)").arg(item.displayDirectory).arg(item.torrents);
        default:
            return {};
        }
    }

    int DownloadDirectoriesModel::rowCount(const QModelIndex&) const { return static_cast<int>(mDirectories.size()); }

    bool DownloadDirectoriesModel::removeRows(int row, int count, const QModelIndex& parent) {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first = mDirectories.begin() + row;
        mDirectories.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    QModelIndex DownloadDirectoriesModel::indexForDirectory(const QString& downloadDirectory) const {
        for (size_t i = 0, max = mDirectories.size(); i < max; ++i) {
            const auto& item = mDirectories[i];
            if (item.directory == downloadDirectory) {
                return index(static_cast<int>(i));
            }
        }
        return {};
    }

    QModelIndex DownloadDirectoriesModel::indexForTorrentsProxyModelFilter() const {
        if (!torrentsProxyModel()) {
            return {};
        }
        return indexForDirectory(torrentsProxyModel()->downloadDirectoryFilter());
    }

    void DownloadDirectoriesModel::resetTorrentsProxyModelFilter() const {
        if (torrentsProxyModel()) {
            torrentsProxyModel()->setDownloadDirectoryFilter({});
        }
    }

    class DownloadDirectoriesModelUpdater final : public ModelListUpdater<
                                                      DownloadDirectoriesModel,
                                                      DownloadDirectoriesModel::DirectoryItem,
                                                      std::vector<DownloadDirectoriesModel::DirectoryItem>> {
    public:
        inline explicit DownloadDirectoriesModelUpdater(DownloadDirectoriesModel& model) : ModelListUpdater(model) {}

    protected:
        std::vector<DownloadDirectoriesModel::DirectoryItem>::iterator findNewItemForItem(
            std::vector<DownloadDirectoriesModel::DirectoryItem>& newItems,
            const DownloadDirectoriesModel::DirectoryItem& item
        ) override {
            return std::find_if(newItems.begin(), newItems.end(), [&](const auto& newItem) {
                return newItem.directory == item.directory;
            });
        }

        bool updateItem(
            DownloadDirectoriesModel::DirectoryItem& item,
            // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
            DownloadDirectoriesModel::DirectoryItem&& newItem
        ) override {
            if (item.torrents != newItem.torrents) {
                item.torrents = newItem.torrents;
                return true;
            }
            return false;
        }

        DownloadDirectoriesModel::DirectoryItem createItemFromNewItem(DownloadDirectoriesModel::DirectoryItem&& newItem
        ) override {
            return DownloadDirectoriesModel::DirectoryItem{std::move(newItem)};
        }
    };

    void DownloadDirectoriesModel::update() {
        std::vector<DirectoryItem> directories;
        directories.push_back({.directory = {}, .displayDirectory = {}, .torrents = rpc()->torrentsCount()});
        for (const auto& torrent : rpc()->torrents()) {
            const QString& directory = torrent->data().downloadDirectory;
            auto found = std::find_if(directories.begin(), directories.end(), [&](const auto& item) {
                return item.directory == directory;
            });
            if (found == directories.end()) {
                directories.push_back(
                    {.directory = directory,
                     .displayDirectory = toNativeSeparators(directory, rpc()->serverSettings()->data().pathOs),
                     .torrents = 1}
                );
            } else {
                ++(found->torrents);
            }
        }

        DownloadDirectoriesModelUpdater updater(*this);
        updater.update(mDirectories, std::move(directories));
    }
}
