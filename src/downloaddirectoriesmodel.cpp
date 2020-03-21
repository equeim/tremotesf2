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

#include <unordered_map>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "libtremotesf/stdutils.h"
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

    DownloadDirectoriesModel::DownloadDirectoriesModel(Rpc* rpc, TorrentsProxyModel* torrentsProxyModel, QObject* parent)
        : QAbstractListModel(parent),
          mRpc(nullptr),
          mTorrentsProxyModel(nullptr)
    {
        setRpc(rpc);
        setTorrentsProxyModel(torrentsProxyModel);

        if (mRpc && mTorrentsProxyModel) {
            update();
        }
    }

#ifdef TREMOTESF_SAILFISHOS
    void DownloadDirectoriesModel::classBegin()
    {
    }

    void DownloadDirectoriesModel::componentComplete()
    {
        if (mRpc && mTorrentsProxyModel) {
            update();
        }
    }
#endif

    QVariant DownloadDirectoriesModel::data(const QModelIndex& index, int role) const
    {
#ifdef TREMOTESF_SAILFISHOS
        if (index.row() == 0) {
            switch (role) {
            case DirectoryRole:
                return QString();
            case TorrentsRole:
                return mRpc->torrentsCount();
            }
        } else {
            const DirectoryItem& item = mDirectories[static_cast<size_t>(index.row() - 1)];
            switch (role) {
            case DirectoryRole:
                return item.directory;
            case TorrentsRole:
                return item.torrents;
            }
        }
#else
        if (index.row() == 0) {
            switch (role) {
            case Qt::DecorationRole:
                return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            case Qt::DisplayRole:
                return qApp->translate("tremotesf", "All (%L1)", "All trackers, %L1 - torrents count").arg(mRpc->torrentsCount());
            }
        } else {
            const DirectoryItem& item = mDirectories[static_cast<size_t>(index.row() - 1)];
            switch (role) {
            case Qt::DecorationRole:
                return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            case Qt::DisplayRole:
                //: %1 is a string (directory name or tracker domain name), %L2 is number of torrents
                return qApp->translate("tremotesf", "%1 (%L2)").arg(item.directory).arg(item.torrents);
            case DirectoryRole:
                return item.directory;
            }
        }
#endif
        return QVariant();
    }

    int DownloadDirectoriesModel::rowCount(const QModelIndex&) const
    {
        return mDirectories.size() + 1;
    }

    bool DownloadDirectoriesModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first(mDirectories.begin() + row - 1);
        mDirectories.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    Rpc* DownloadDirectoriesModel::rpc() const
    {
        return mRpc;
    }

    void DownloadDirectoriesModel::setRpc(Rpc* rpc)
    {
        if (rpc && !mRpc) {
            mRpc = rpc;
            QObject::connect(mRpc, &Rpc::torrentsUpdated, this, &DownloadDirectoriesModel::update);
        }
    }

    TorrentsProxyModel* DownloadDirectoriesModel::torrentsProxyModel() const
    {
        return mTorrentsProxyModel;
    }

    void DownloadDirectoriesModel::setTorrentsProxyModel(TorrentsProxyModel* model)
    {
        if (model && !mTorrentsProxyModel) {
            mTorrentsProxyModel = model;
        }
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
        if (mRpc->torrents().empty()) {
            if (!mDirectories.empty()) {
                const QModelIndex firstIndex(index(0));
                emit dataChanged(firstIndex, firstIndex);

                beginRemoveRows(QModelIndex(), 1, static_cast<int>(mDirectories.size()));
                mDirectories.clear();
                endRemoveRows();

                mTorrentsProxyModel->setTracker(QString());
            }
            return;
        }

        std::unordered_map<QString, int> directories;
        for (const auto& torrent : mRpc->torrents()) {
            const QString& directory = torrent->downloadDirectory();
            auto found = directories.find(directory);
            if (found == directories.end()) {
                directories.emplace(directory, 1);
            } else {
                ++(found->second);
            }
        }
        const auto directoriesEnd(directories.end());

        {
            ModelBatchRemover modelRemover(this);
            for (int i = static_cast<int>(mDirectories.size()) - 1; i >= 0; --i) {
                const auto found(directories.find(mDirectories[static_cast<size_t>(i)].directory));
                if (found == directoriesEnd) {
                    modelRemover.remove(i + 1);
                }
            }
            modelRemover.remove();
        }

        mDirectories.reserve(directories.size());

        ModelBatchChanger changer(this);
        for (int i = 0, max = static_cast<int>(mDirectories.size()); i < max; ++i) {
            DirectoryItem& item = mDirectories[static_cast<size_t>(i)];
            const auto found(directories.find(item.directory));
            if (found != directoriesEnd) {
                item.torrents = found->second;
                changer.changed(i + 1);
                directories.erase(found);
            }
        }
        changer.changed();

        if (!directories.empty()) {
            const int firstRow = static_cast<int>(mDirectories.size() + 1);
            beginInsertRows(QModelIndex(), firstRow, firstRow + static_cast<int>(directories.size()) - 1);
            for (const auto& i : directories) {
                const QString& tracker = i.first;
                const int torrents = i.second;
                mDirectories.push_back(DirectoryItem{std::move(tracker), torrents});
            }
            endInsertRows();
        }
    }
}
