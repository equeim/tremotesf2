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

#include <unordered_map>

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "rpc.h"
#include "stdutils.h"
#include "torrent.h"
#include "torrentsproxymodel.h"
#include "tracker.h"

namespace tremotesf
{
#ifdef TREMOTESF_SAILFISHOS
    namespace
    {
        enum Role
        {
            TrackerRole = Qt::UserRole,
            TorrentsRole
        };
    }
#endif

    AllTrackersModel::AllTrackersModel(Rpc* rpc, TorrentsProxyModel* torrentsProxyModel, QObject* parent)
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
    void AllTrackersModel::classBegin()
    {
    }

    void AllTrackersModel::componentComplete()
    {
        if (mRpc && mTorrentsProxyModel) {
            update();
        }
    }
#endif

    QVariant AllTrackersModel::data(const QModelIndex& index, int role) const
    {
#ifdef TREMOTESF_SAILFISHOS
        if (index.row() == 0) {
            switch (role) {
            case TrackerRole:
                return QString();
            case TorrentsRole:
                return mRpc->torrentsCount();
            }
        } else {
            const int row = index.row() - 1;
            switch (role) {
            case TrackerRole:
                return mTrackers[row];
            case TorrentsRole:
                return mTrackersTorrents[row];
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
                return QIcon::fromTheme(QLatin1String("network-server"));
            case Qt::DisplayRole:
                return qApp->translate("tremotesf", "%1 (%2)").arg(mTrackers[row]).arg(mTrackersTorrents[row]);
            case TrackerRole:
                return mTrackers[row];
            }
        }
#endif
        return QVariant();
    }

    int AllTrackersModel::rowCount(const QModelIndex&) const
    {
        return mTrackers.size() + 1;
    }

    Rpc* AllTrackersModel::rpc() const
    {
        return mRpc;
    }

    void AllTrackersModel::setRpc(Rpc* rpc)
    {
        if (rpc && !mRpc) {
            mRpc = rpc;
            QObject::connect(mRpc, &Rpc::torrentsUpdated, this, &AllTrackersModel::update);
        }
    }

    TorrentsProxyModel* AllTrackersModel::torrentsProxyModel() const
    {
        return mTorrentsProxyModel;
    }

    void AllTrackersModel::setTorrentsProxyModel(TorrentsProxyModel* model)
    {
        if (model && !mTorrentsProxyModel) {
            mTorrentsProxyModel = model;
        }
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> AllTrackersModel::roleNames() const
    {
        return {{TrackerRole, "tracker"},
                {TorrentsRole, "torrents"}};
    }
#endif

    void AllTrackersModel::update()
    {
        if (mRpc->torrents().empty()) {
            if (!mTrackers.empty()) {
                const QModelIndex firstIndex(index(0));
                emit dataChanged(firstIndex, firstIndex);

                beginRemoveRows(QModelIndex(), 1, mTrackers.size());
                mTrackers.clear();
                mTrackersTorrents.clear();
                endRemoveRows();

                mTorrentsProxyModel->setTracker(QString());
            }
            return;
        }

        std::unordered_map<QString, int> trackers;
        for (const std::shared_ptr<Torrent>& torrent : mRpc->torrents()) {
            for (const std::shared_ptr<Tracker>& tracker : torrent->trackers()) {
                const QString& site = tracker->site();

                auto found = trackers.find(site);
                if (found == trackers.end()) {
                    trackers.insert({site, 1});
                } else {
                    ++(found->second);
                }
            }
        }

        for (int i = 0, max = mTrackers.size(); i < max; ++i) {
            if (trackers.find(mTrackers[i]) == trackers.end()) {
                if (mTrackers[i] == mTorrentsProxyModel->tracker()) {
                    mTorrentsProxyModel->setTracker(QString());
                }

                const int row = i + 1;
                beginRemoveRows(QModelIndex(), row, row);
                mTrackers.erase(mTrackers.begin() + i);
                mTrackersTorrents.erase(mTrackersTorrents.begin() + i);
                endRemoveRows();
                i--;
                max--;
            }
        }

        for (auto i = trackers.cbegin(), max = trackers.cend();
             i != max;
             ++i) {

            const QString& tracker = i->first;
            auto row = index_of(mTrackers, tracker);
            if (row == mTrackers.size()) {
                row = mTrackers.size() + 1;
                beginInsertRows(QModelIndex(), row, row);
                mTrackers.push_back(tracker);
                mTrackersTorrents.push_back(i->second);
                endInsertRows();
            } else {
                mTrackersTorrents[row] = i->second;
                const QModelIndex modelIndex(index(row + 1));
                emit dataChanged(modelIndex, modelIndex);
            }
        }

        emit dataChanged(index(0), index(mTrackers.size()));
    }
}
