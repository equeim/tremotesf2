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

#include "downloaddirectoriesmodel.h"

#include <map>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "libtremotesf/qtsupport.h"
#include "libtremotesf/torrent.h"

#include "modelutils.h"
#include "torrentsproxymodel.h"
#include "trpc.h"

namespace tremotesf
{
#ifdef TREMOTESF_SAILFISHOS
    namespace
    {
        enum Role
        {
            DirectoryRole = Qt::UserRole,
            TorrentsRole
        };
    }
#endif

    QVariant DownloadDirectoriesModel::data(const QModelIndex& index, int role) const
    {
        const DirectoryItem& item = mDirectories[static_cast<size_t>(index.row())];
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case DirectoryRole:
            return item.directory;
        case TorrentsRole:
            return item.torrents;
        }
#else
        switch (role) {
        case Qt::DecorationRole:
            return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (item.directory.isEmpty()) {
                return qApp->translate("tremotesf", "All (%L1)", "All trackers, %L1 - torrents count").arg(item.torrents);
            }
            //: %1 is a string (directory name or tracker domain name), %L2 is number of torrents
            return qApp->translate("tremotesf", "%1 (%L2)").arg(item.directory).arg(item.torrents);
        case DirectoryRole:
            return item.directory;
        }
#endif
        return {};
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

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> DownloadDirectoriesModel::roleNames() const
    {
        return {{DirectoryRole, "directory"},
                {TorrentsRole, "torrents"}};
    }
#endif

    void DownloadDirectoriesModel::update()
    {
        if (rpc()->torrents().empty()) {
            if (!mDirectories.empty()) {
                mDirectories[0].torrents = 0;
                const auto firstIndex = index(0);
                emit dataChanged(firstIndex, firstIndex);

                beginRemoveRows({}, 1, static_cast<int>(mDirectories.size() - 1));
                mDirectories.erase(mDirectories.begin() + 1, mDirectories.end());
                endRemoveRows();
            }
            return;
        }

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
        const auto directoriesEnd(directories.end());

        if (!mDirectories.empty()) {
            ModelBatchRemover modelRemover(this);
            size_t i = (mDirectories.size() - 1);
            while (i != 0) {
                const auto found = directories.find(mDirectories[i].directory);
                if (found == directoriesEnd) {
                    modelRemover.remove(static_cast<int>(i));
                }
                --i;
            }
            modelRemover.remove();
        }

        ModelBatchChanger changer(this);
        for (size_t i = 0, max = mDirectories.size(); i < max; ++i) {
            DirectoryItem& item = mDirectories[i];
            const auto found = directories.find(item.directory);
            if (found != directoriesEnd) {
                const int torrents = found->second;
                if (torrents != item.torrents) {
                    item.torrents = found->second;
                    changer.changed(static_cast<int>(i));
                }
                directories.erase(found);
            }
        }
        changer.changed();

        if (!directories.empty()) {
            const int firstRow = static_cast<int>(mDirectories.size());
            beginInsertRows({}, firstRow, firstRow + static_cast<int>(directories.size()) - 1);
            mDirectories.reserve(mDirectories.size() + directories.size());
            for (const auto& i : directories) {
                const QString& tracker = i.first;
                const int torrents = i.second;
                mDirectories.push_back(DirectoryItem{tracker, torrents});
            }
            endInsertRows();
        }
    }
}
