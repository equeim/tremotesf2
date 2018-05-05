/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "rpc.h"
#include "stdutils.h"
#include "torrent.h"
#include "torrentsproxymodel.h"

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
            const int row = index.row() - 1;
            switch (role) {
            case DirectoryRole:
                return mDirectories[row];
            case TorrentsRole:
                return mDirectoriesTorrents[row];
            }
        }
#else
        if (index.row() == 0) {
            switch (role) {
            case Qt::DecorationRole:
                return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            case Qt::DisplayRole:
                return qApp->translate("tremotesf", "All (%1)", "All trackers, %1 - torrents count").arg(mRpc->torrentsCount());
            }
        } else {
            const int row = index.row() - 1;
            switch (role) {
            case Qt::DecorationRole:
                return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            case Qt::DisplayRole:
                return qApp->translate("tremotesf", "%1 (%2)").arg(mDirectories[row]).arg(mDirectoriesTorrents[row]);
            case DirectoryRole:
                return mDirectories[row];
            }
        }
#endif
        return QVariant();
    }

    int DownloadDirectoriesModel::rowCount(const QModelIndex&) const
    {
        return mDirectories.size() + 1;
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

                beginRemoveRows(QModelIndex(), 1, mDirectories.size());
                mDirectories.clear();
                mDirectoriesTorrents.clear();
                endRemoveRows();

                mTorrentsProxyModel->setTracker(QString());
            }
            return;
        }

        std::unordered_map<QString, int> directories;
        for (const std::shared_ptr<Torrent>& torrent : mRpc->torrents()) {
            const QString& directory = torrent->downloadDirectory();

            auto found = directories.find(directory);
            if (found == directories.end()) {
                directories.insert({directory, 1});
            } else {
                ++(found->second);
            }
        }

        for (int i = 0, max = mDirectories.size(); i < max; ++i) {
            if (directories.find(mDirectories[i]) == directories.end()) {
                if (mDirectories[i] == mTorrentsProxyModel->tracker()) {
                    mTorrentsProxyModel->setTracker(QString());
                }

                const int row = i + 1;
                beginRemoveRows(QModelIndex(), row, row);
                mDirectories.erase(mDirectories.begin() + i);
                mDirectoriesTorrents.erase(mDirectoriesTorrents.begin() + i);
                endRemoveRows();
                i--;
                max--;
            }
        }

        for (auto i = directories.cbegin(), max = directories.cend();
             i != max;
             ++i) {

            const QString& tracker = i->first;
            auto row = index_of(mDirectories, tracker);
            if (row == mDirectories.size()) {
                row = mDirectories.size() + 1;
                beginInsertRows(QModelIndex(), row, row);
                mDirectories.push_back(tracker);
                mDirectoriesTorrents.push_back(i->second);
                endInsertRows();
            } else {
                mDirectoriesTorrents[row] = i->second;
                const QModelIndex modelIndex(index(row + 1));
                emit dataChanged(modelIndex, modelIndex);
            }
        }

        emit dataChanged(index(0), index(mDirectories.size()));
    }
}
