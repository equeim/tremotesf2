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

#include <QCoreApplication>
#include <QPixmap>

#include "libtremotesf/stdutils.h"
#include "libtremotesf/torrent.h"

#include "modelutils.h"
#include "trpc.h"
#include "utils.h"

#ifndef TREMOTESF_SAILFISHOS
#include "desktop/desktoputils.h"
#endif

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
#ifdef TREMOTESF_SAILFISHOS
        return 1;
#else
        return ColumnCount;
#endif
    }

    QVariant TorrentsModel::data(const QModelIndex& index, int role) const
    {
        Torrent* torrent = mTorrents[static_cast<size_t>(index.row())].get();

#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case NameRole:
            return torrent->name();
        case StatusRole:
            return torrent->status();
        case TotalSizeRole:
            return torrent->totalSize();
        case PercentDoneRole:
            return torrent->percentDone();
        case EtaRole:
            return torrent->eta();
        case RatioRole:
            return torrent->ratio();
        case AddedDateRole:
            return torrent->addedDate();
        case IdRole:
            return torrent->id();
        case TorrentRole:
            return QVariant::fromValue(torrent);
        }
#else
        switch (role) {
        case Qt::DecorationRole:
            if (index.column() == NameColumn) {
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
            switch (index.column()) {
            case NameColumn:
                return torrent->name();
            case SizeWhenDoneColumn:
                return Utils::formatByteSize(torrent->sizeWhenDone());
            case TotalSizeColumn:
                return Utils::formatByteSize(torrent->sizeWhenDone());
            case ProgressColumn:
                if (torrent->status() == TorrentData::Checking) {
                    return Utils::formatProgress(torrent->recheckProgress());
                }
                return Utils::formatProgress(torrent->percentDone());
            case StatusColumn:
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
            case QueuePositionColumn:
                return torrent->queuePosition();
            case SeedersColumn:
                return torrent->seeders();
            case LeechersColumn:
                return torrent->leechers();
            case DownloadSpeedColumn:
                return Utils::formatByteSpeed(torrent->downloadSpeed());
            case UploadSpeedColumn:
                return Utils::formatByteSpeed(torrent->uploadSpeed());
            case EtaColumn:
                return Utils::formatEta(torrent->eta());
            case RatioColumn:
                return Utils::formatRatio(torrent->ratio());
            case AddedDateColumn:
                return torrent->addedDate();
            case DoneDateColumn:
                return torrent->doneDate();
            case DownloadSpeedLimitColumn:
                if (torrent->isDownloadSpeedLimited()) {
                    return Utils::formatSpeedLimit(torrent->downloadSpeedLimit());
                }
                break;
            case UploadSpeedLimitColumn:
                if (torrent->isUploadSpeedLimited()) {
                    return Utils::formatSpeedLimit(torrent->uploadSpeedLimit());
                }
                break;
            case TotalDownloadedColumn:
                return Utils::formatByteSize(torrent->totalDownloaded());
            case TotalUploadedColumn:
                return Utils::formatByteSize(torrent->totalUploaded());
            case LeftUntilDoneColumn:
                return Utils::formatByteSize(torrent->leftUntilDone());
            case DownloadDirectoryColumn:
                return torrent->downloadDirectory();
            case CompletedSizeColumn:
                return Utils::formatByteSize(torrent->completedSize());
            case ActivityDateColumn:
                return torrent->activityDate();
            }
            break;
        case SortRole:
            switch (index.column()) {
            case SizeWhenDoneColumn:
                return torrent->sizeWhenDone();
            case TotalSizeColumn:
                return torrent->totalSize();
            case ProgressBarColumn:
            case ProgressColumn:
                if (torrent->status() == TorrentData::Checking) {
                    return torrent->recheckProgress();
                }
                return torrent->percentDone();
            case StatusColumn:
                return torrent->status();
            case DownloadSpeedColumn:
                return torrent->downloadSpeed();
            case UploadSpeedColumn:
                return torrent->uploadSpeed();
            case EtaColumn:
                return torrent->eta();
            case RatioColumn:
                return torrent->ratio();
            case AddedDateColumn:
                return torrent->addedDate();
            case DoneDateColumn:
                return torrent->doneDate();
            case DownloadSpeedLimitColumn:
                if (torrent->isDownloadSpeedLimited()) {
                    return torrent->downloadSpeedLimit();
                }
                return -1;
            case UploadSpeedLimitColumn:
                if (torrent->isUploadSpeedLimited()) {
                    return torrent->uploadSpeedLimit();
                }
                return -1;
            case TotalDownloadedColumn:
                return torrent->totalDownloaded();
            case TotalUploadedColumn:
                return torrent->totalUploaded();
            case LeftUntilDoneColumn:
                return torrent->leftUntilDone();
            case CompletedSizeColumn:
                return torrent->completedSize();
            case ActivityDateColumn:
                return torrent->activityDate();
            default:
                return data(index, Qt::DisplayRole);
            }
        }
#endif

        return QVariant();
    }

#ifndef TREMOTESF_SAILFISHOS
    QVariant TorrentsModel::headerData(int section, Qt::Orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case NameColumn:
                return qApp->translate("tremotesf", "Name");
            case SizeWhenDoneColumn:
                return qApp->translate("tremotesf", "Size");
            case TotalSizeColumn:
                return qApp->translate("tremotesf", "Total Size");
            case ProgressBarColumn:
                return qApp->translate("tremotesf", "Progress Bar");
            case ProgressColumn:
                return qApp->translate("tremotesf", "Progress");
            case PriorityColumn:
                return qApp->translate("tremotesf", "Priority");
            case QueuePositionColumn:
                return qApp->translate("tremotesf", "Queue Position");
            case StatusColumn:
                return qApp->translate("tremotesf", "Status");
            case SeedersColumn:
                return qApp->translate("tremotesf", "Seeders");
            case LeechersColumn:
                return qApp->translate("tremotesf", "Leechers");
            case DownloadSpeedColumn:
                return qApp->translate("tremotesf", "Down Speed");
            case UploadSpeedColumn:
                return qApp->translate("tremotesf", "Up Speed");
            case EtaColumn:
                return qApp->translate("tremotesf", "ETA");
            case RatioColumn:
                return qApp->translate("tremotesf", "Ratio");
            case AddedDateColumn:
                return qApp->translate("tremotesf", "Added on");
            case DoneDateColumn:
                return qApp->translate("tremotesf", "Completed on");
            case DownloadSpeedLimitColumn:
                return qApp->translate("tremotesf", "Down Limit");
            case UploadSpeedLimitColumn:
                return qApp->translate("tremotesf", "Up Limit");
            case TotalDownloadedColumn:
                //: Torrent's downloaded size
                return qApp->translate("tremotesf", "Downloaded");
            case TotalUploadedColumn:
                //: Torrent's uploaded size
                return qApp->translate("tremotesf", "Uploaded");
            case LeftUntilDoneColumn:
                //: Torrents's remaining size
                return qApp->translate("tremotesf", "Remaining");
            case DownloadDirectoryColumn:
                return qApp->translate("tremotesf", "Download Directory");
            case CompletedSizeColumn:
                //: Torrent's completed size
                return qApp->translate("tremotesf", "Completed");
            case ActivityDateColumn:
                return qApp->translate("tremotesf", "Last Activity");
            }
        }

        return QVariant();
    }
#endif

    int TorrentsModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(mTorrents.size());
    }

    bool TorrentsModel::removeRows(int row, int count, const QModelIndex& parent)
    {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first(mTorrents.begin() + row);
        mTorrents.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    Rpc* TorrentsModel::rpc() const
    {
        return mRpc;
    }

    void TorrentsModel::setRpc(Rpc* rpc)
    {
        if (rpc && !mRpc) {
            mRpc = rpc;
            update({}, {}, static_cast<int>(mRpc->torrents().size()));
            QObject::connect(mRpc, &Rpc::torrentsUpdated, this, &TorrentsModel::update);
        }
    }

    Torrent* TorrentsModel::torrentAtIndex(const QModelIndex& index) const
    {
        return torrentAtRow(index.row());
    }

    Torrent* TorrentsModel::torrentAtRow(int row) const
    {
        return mTorrents[static_cast<size_t>(row)].get();
    }

    QVariantList TorrentsModel::idsFromIndexes(const QModelIndexList& indexes) const
    {
        QVariantList ids;
        ids.reserve(indexes.size());
        for (const QModelIndex& index : indexes) {
            ids.append(mTorrents[static_cast<size_t>(index.row())]->id());
        }
        return ids;
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> TorrentsModel::roleNames() const
    {
        return {{NameRole, "name"},
                {StatusRole, "status"},
                {TotalSizeRole, "totalSize"},
                {PercentDoneRole, "percentDone"},
                {EtaRole, "eta"},
                {RatioRole, "ratio"},
                {AddedDateRole, "addedDate"},
                {IdRole, "id"},
                {TorrentRole, "torrent"}};
    }
#endif

    void TorrentsModel::update(const std::vector<int>& removed, const std::vector<int>& changed, int added)
    {
        if (!removed.empty()) {
            ModelBatchRemover remover{this};
            for (int index : removed) {
                remover.remove(index);
            }
            remover.remove();
        }

        if (!changed.empty()) {
            ModelBatchChanger changer{this};
            for (int index : changed) {
                changer.changed(index);
            }
            changer.changed();
        }

        if (added > 0) {
            const int first = static_cast<int>(mTorrents.size());
            const int last = first + added - 1;
            beginInsertRows(QModelIndex(), first, last);
            mTorrents.reserve(static_cast<size_t>(last + 1));

            for (auto end = mRpc->torrents().end(), i = end - added; i != end; ++i) {
                const auto& torrent = *i;
                mTorrents.push_back(torrent);

                const Torrent* torrentPointer = torrent.get();
                QObject::connect(torrentPointer, &Torrent::limitsEdited, this, [=] {
                    for (int i = 0, max = static_cast<int>(mTorrents.size()); i < max; ++i) {
                        if (mTorrents[static_cast<size_t>(i)].get() == torrentPointer) {
                            emit dataChanged(index(i, 0), index(i, columnCount() - 1));
                            break;
                        }
                    }
                });
            }

            endInsertRows();
        }
    }
}
