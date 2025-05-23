// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "downloaddirectoriesmodel.h"

#include <algorithm>

#include <QCoreApplication>
#include <QIcon>

#include "rpc/pathutils.h"
#include "rpc/rpc.h"
#include "rpc/torrent.h"
#include "rpc/serversettings.h"
#include "ui/itemmodels/modelutils.h"
#include "ui/screens/mainwindow/downloaddirectoriesmodel.h"

#include "desktoputils.h"
#include "settings.h"
#include "torrentsproxymodel.h"

namespace tremotesf {
    DownloadDirectoriesModel::DownloadDirectoriesModel(QObject* parent) : BaseTorrentsFiltersSettingsModel(parent) {
        const auto settings = Settings::instance();
        mDisplayFullDownloadDirectoryPath = settings->get_displayFullDownloadDirectoryPath();
        QObject::connect(settings, &Settings::displayFullDownloadDirectoryPathChanged, this, [this, settings] {
            mDisplayFullDownloadDirectoryPath = settings->get_displayFullDownloadDirectoryPath();
        });
    }

    QVariant DownloadDirectoriesModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const DirectoryItem& item = mDirectories.at(static_cast<size_t>(index.row()));
        switch (role) {
        case static_cast<int>(Role::Directory):
            return item.directory;
        case Qt::DecorationRole: {
            return desktoputils::standardDirIcon();
        }
        case Qt::DisplayRole: {
            if (item.directory.isEmpty()) {
                //: Filter option of torrents list's download directory filter. %L1 is total number of torrents
                return qApp->translate("tremotesf", "All (%L1)").arg(item.torrents);
            }
            //: Filter option of torrents list's download directory filter. %1 is download directory, %L2 is number of torrents with that download directory
            const auto& text =
                mDisplayFullDownloadDirectoryPath ? item.displayDirectory : lastPathSegment(item.directory);
            return qApp->translate("tremotesf", "%1 (%L2)").arg(text).arg(item.torrents);
        }
        case Qt::ToolTipRole:
            if (item.directory.isEmpty()) {
                return data(index, Qt::DisplayRole);
            }
            return item.displayDirectory;
        default:
            return {};
        }
    }

    int DownloadDirectoriesModel::rowCount(const QModelIndex&) const { return static_cast<int>(mDirectories.size()); }

    QModelIndex DownloadDirectoriesModel::indexForTorrentsProxyModelFilter() const {
        if (!torrentsProxyModel()) {
            return {};
        }
        const auto filter = torrentsProxyModel()->downloadDirectoryFilter();
        for (size_t i = 0, max = mDirectories.size(); i < max; ++i) {
            const auto& item = mDirectories[i];
            if (item.directory == filter) {
                return index(static_cast<int>(i));
            }
        }
        return {};
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
            return std::ranges::find(newItems, item.directory, &DownloadDirectoriesModel::DirectoryItem::directory);
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

        DownloadDirectoriesModel::DirectoryItem
        createItemFromNewItem(DownloadDirectoriesModel::DirectoryItem&& newItem) override {
            return DownloadDirectoriesModel::DirectoryItem{std::move(newItem)};
        }
    };

    void DownloadDirectoriesModel::update() {
        std::vector<DirectoryItem> directories;
        directories.push_back({.torrents = rpc()->torrentsCount()});
        for (const auto& torrent : rpc()->torrents()) {
            const QString& directory = torrent->data().downloadDirectory;
            auto found = std::ranges::find(directories, directory, &DirectoryItem::directory);
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
