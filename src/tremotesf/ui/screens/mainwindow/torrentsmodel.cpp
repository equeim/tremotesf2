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

#include "torrentsmodel.h"

#include <limits>

#include <QCoreApplication>
#include <QMetaEnum>
#include <QPixmap>

#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/desktoputils.h"
#include "tremotesf/utils.h"

namespace tremotesf
{
    using libtremotesf::Torrent;
    using libtremotesf::TorrentData;

    TorrentsModel::TorrentsModel(Rpc* rpc, QObject* parent)
        : QAbstractTableModel(parent),
          mRpc(nullptr)
    {
        setRpc(rpc);
    }

    int TorrentsModel::columnCount(const QModelIndex&) const
    {
        return QMetaEnum::fromType<Column>().keyCount();
    }

    QVariant TorrentsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid()) {
            return {};
        }
        Torrent* torrent = mRpc->torrents()[static_cast<size_t>(index.row())].get();
        switch (role) {
        case Qt::DecorationRole:
            if (static_cast<Column>(index.column()) == Column::Name) {
                using namespace desktoputils;
                switch (torrent->status()) {
                case TorrentData::Paused:
                    return QPixmap(statusIconPath(PausedIcon));
                case TorrentData::Seeding:
                    return QPixmap(statusIconPath(SeedingIcon));
                case TorrentData::Downloading:
                    return QPixmap(statusIconPath(DownloadingIcon));
                case TorrentData::StalledDownloading:
                    return QPixmap(statusIconPath(StalledDownloadingIcon));
                case TorrentData::StalledSeeding:
                    return QPixmap(statusIconPath(StalledSeedingIcon));
                case TorrentData::QueuedForDownloading:
                case TorrentData::QueuedForSeeding:
                    return QPixmap(statusIconPath(QueuedIcon));
                case TorrentData::Checking:
                case TorrentData::QueuedForChecking:
                    return QPixmap(statusIconPath(CheckingIcon));
                case TorrentData::Errored:
                    return QPixmap(statusIconPath(ErroredIcon));
                }
            }
            break;
        case Qt::DisplayRole:
            switch (static_cast<Column>(index.column())) {
            case Column::Name:
                return torrent->name();
            case Column::SizeWhenDone:
                return Utils::formatByteSize(torrent->sizeWhenDone());
            case Column::TotalSize:
                return Utils::formatByteSize(torrent->totalSize());
            case Column::Progress:
                if (torrent->status() == TorrentData::Checking) {
                    return Utils::formatProgress(torrent->recheckProgress());
                }
                return Utils::formatProgress(torrent->percentDone());
            case Column::Status:
                switch (torrent->status()) {
                case TorrentData::Paused:
                    return qApp->translate("tremotesf", "Paused", "Torrent status");
                case TorrentData::Downloading:
                case TorrentData::StalledDownloading:
                    return qApp->translate("tremotesf", "Downloading", "Torrent status");
                case TorrentData::Seeding:
                case TorrentData::StalledSeeding:
                    return qApp->translate("tremotesf", "Seeding", "Torrent status");
                case TorrentData::QueuedForDownloading:
                case TorrentData::QueuedForSeeding:
                    return qApp->translate("tremotesf", "Queued", "Torrent status");
                case TorrentData::Checking:
                    return qApp->translate("tremotesf", "Checking", "Torrent status");
                case TorrentData::QueuedForChecking:
                    return qApp->translate("tremotesf", "Queued for checking");
                case TorrentData::Errored:
                    return torrent->errorString();
                }
                break;
            case Column::QueuePosition:
                return torrent->queuePosition();
            case Column::Seeders:
                return torrent->seeders();
            case Column::Leechers:
                return torrent->leechers();
            case Column::DownloadSpeed:
                return Utils::formatByteSpeed(torrent->downloadSpeed());
            case Column::UploadSpeed:
                return Utils::formatByteSpeed(torrent->uploadSpeed());
            case Column::Eta:
                return Utils::formatEta(torrent->eta());
            case Column::Ratio:
                return Utils::formatRatio(torrent->ratio());
            case Column::AddedDate:
                return torrent->addedDate();
            case Column::DoneDate:
                return torrent->doneDate();
            case Column::DownloadSpeedLimit:
                if (torrent->isDownloadSpeedLimited()) {
                    return Utils::formatSpeedLimit(torrent->downloadSpeedLimit());
                }
                break;
            case Column::UploadSpeedLimit:
                if (torrent->isUploadSpeedLimited()) {
                    return Utils::formatSpeedLimit(torrent->uploadSpeedLimit());
                }
                break;
            case Column::TotalDownloaded:
                return Utils::formatByteSize(torrent->totalDownloaded());
            case Column::TotalUploaded:
                return Utils::formatByteSize(torrent->totalUploaded());
            case Column::LeftUntilDone:
                return Utils::formatByteSize(torrent->leftUntilDone());
            case Column::DownloadDirectory:
                return torrent->downloadDirectory();
            case Column::CompletedSize:
                return Utils::formatByteSize(torrent->completedSize());
            case Column::ActivityDate:
                return torrent->activityDate();
            default:
                break;
            }
            break;
        case Qt::ToolTipRole:
            switch (static_cast<Column>(index.column())) {
            case Column::Name:
            case Column::Status:
            case Column::AddedDate:
            case Column::DoneDate:
            case Column::DownloadDirectory:
            case Column::ActivityDate:
                return data(index, Qt::DisplayRole);
            default:
                break;
            }
            break;
        case static_cast<int>(Role::Sort):
            switch (static_cast<Column>(index.column())) {
            case Column::SizeWhenDone:
                return torrent->sizeWhenDone();
            case Column::TotalSize:
                return torrent->totalSize();
            case Column::ProgressBar:
            case Column::Progress:
                if (torrent->status() == TorrentData::Checking) {
                    return torrent->recheckProgress();
                }
                return torrent->percentDone();
            case Column::Status:
                return torrent->status();
            case Column::DownloadSpeed:
                return torrent->downloadSpeed();
            case Column::UploadSpeed:
                return torrent->uploadSpeed();
            case Column::Eta:
            {
                const auto eta = torrent->eta();
                if (eta < 0) {
                    return std::numeric_limits<decltype(eta)>::max();
                }
                return eta;
            }
            case Column::Ratio:
                return torrent->ratio();
            case Column::AddedDate:
                return torrent->addedDate();
            case Column::DoneDate:
                return torrent->doneDate();
            case Column::DownloadSpeedLimit:
                if (torrent->isDownloadSpeedLimited()) {
                    return torrent->downloadSpeedLimit();
                }
                return -1;
            case Column::UploadSpeedLimit:
                if (torrent->isUploadSpeedLimited()) {
                    return torrent->uploadSpeedLimit();
                }
                return -1;
            case Column::TotalDownloaded:
                return torrent->totalDownloaded();
            case Column::TotalUploaded:
                return torrent->totalUploaded();
            case Column::LeftUntilDone:
                return torrent->leftUntilDone();
            case Column::CompletedSize:
                return torrent->completedSize();
            case Column::ActivityDate:
                return torrent->activityDate();
            default:
                return data(index, Qt::DisplayRole);
            }
        case static_cast<int>(Role::TextElideMode):
            if (static_cast<Column>(index.column()) == Column::DownloadDirectory) {
                return Qt::ElideMiddle;
            }
            return Qt::ElideRight;
        }
        return {};
    }

    QVariant TorrentsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (static_cast<Column>(section)) {
        case Column::Name:
            return qApp->translate("tremotesf", "Name");
        case Column::SizeWhenDone:
            return qApp->translate("tremotesf", "Size");
        case Column::TotalSize:
            return qApp->translate("tremotesf", "Total Size");
        case Column::ProgressBar:
            return qApp->translate("tremotesf", "Progress Bar");
        case Column::Progress:
            return qApp->translate("tremotesf", "Progress");
        case Column::Priority:
            return qApp->translate("tremotesf", "Priority");
        case Column::QueuePosition:
            return qApp->translate("tremotesf", "Queue Position");
        case Column::Status:
            return qApp->translate("tremotesf", "Status");
        case Column::Seeders:
            return qApp->translate("tremotesf", "Seeders");
        case Column::Leechers:
            return qApp->translate("tremotesf", "Leechers");
        case Column::DownloadSpeed:
            return qApp->translate("tremotesf", "Down Speed");
        case Column::UploadSpeed:
            return qApp->translate("tremotesf", "Up Speed");
        case Column::Eta:
            return qApp->translate("tremotesf", "ETA");
        case Column::Ratio:
            return qApp->translate("tremotesf", "Ratio");
        case Column::AddedDate:
            return qApp->translate("tremotesf", "Added on");
        case Column::DoneDate:
            return qApp->translate("tremotesf", "Completed on");
        case Column::DownloadSpeedLimit:
            return qApp->translate("tremotesf", "Down Limit");
        case Column::UploadSpeedLimit:
            return qApp->translate("tremotesf", "Up Limit");
        case Column::TotalDownloaded:
            //: Torrent's downloaded size
            return qApp->translate("tremotesf", "Downloaded");
        case Column::TotalUploaded:
            //: Torrent's uploaded size
            return qApp->translate("tremotesf", "Uploaded");
        case Column::LeftUntilDone:
            //: Torrents's remaining size
            return qApp->translate("tremotesf", "Remaining");
        case Column::DownloadDirectory:
            return qApp->translate("tremotesf", "Download Directory");
        case Column::CompletedSize:
            //: Torrent's completed size
            return qApp->translate("tremotesf", "Completed");
        case Column::ActivityDate:
            return qApp->translate("tremotesf", "Last Activity");
        default:
            return {};
        }
    }

    int TorrentsModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mRpc->torrentsCount());
    }

    Rpc* TorrentsModel::rpc() const
    {
        return mRpc;
    }

    void TorrentsModel::setRpc(Rpc* rpc)
    {
        if (rpc != mRpc) {
            if (mRpc) {
                QObject::disconnect(mRpc, nullptr, this, nullptr);
            }
            mRpc = rpc;
            if (rpc) {
                QObject::connect(rpc, &Rpc::onAboutToAddTorrents, this, [=](size_t count) {
                    const auto first = mRpc->torrentsCount();
                    beginInsertRows({}, first, first + static_cast<int>(count) - 1);
                });

                QObject::connect(rpc, &Rpc::onAddedTorrents, this, [=] {
                    endInsertRows();
                });

                QObject::connect(rpc, &Rpc::onAboutToRemoveTorrents, this, [=](size_t first, size_t last) {
                    beginRemoveRows({}, static_cast<int>(first), static_cast<int>(last - 1));
                });

                QObject::connect(rpc, &Rpc::onRemovedTorrents, this, [=] {
                    endRemoveRows();
                });

                QObject::connect(rpc, &Rpc::onChangedTorrents, this, [=](size_t first, size_t last) {
                    emit dataChanged(index(static_cast<int>(first), 0), index(static_cast<int>(last), columnCount() - 1));
                });

                const auto count = rpc->torrentsCount();
                if (count != 0) {
                    beginInsertRows({}, 0, count - 1);
                    endInsertRows();
                }
            }
        }
    }

    Torrent* TorrentsModel::torrentAtIndex(const QModelIndex& index) const
    {
        return torrentAtRow(index.row());
    }

    Torrent* TorrentsModel::torrentAtRow(int row) const
    {
        return mRpc->torrents()[static_cast<size_t>(row)].get();
    }

    QVariantList TorrentsModel::idsFromIndexes(const QModelIndexList& indexes) const
    {
        QVariantList ids;
        ids.reserve(indexes.size());
        for (const QModelIndex& index : indexes) {
            ids.append(torrentAtIndex(index)->id());
        }
        return ids;
    }
}
