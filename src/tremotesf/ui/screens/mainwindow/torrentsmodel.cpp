// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentsmodel.h"

#include <limits>

#include <QCoreApplication>
#include <QMetaEnum>
#include <QPixmap>

#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/desktoputils.h"
#include "tremotesf/utils.h"

namespace tremotesf {
    using libtremotesf::Torrent;
    using libtremotesf::TorrentData;

    TorrentsModel::TorrentsModel(Rpc* rpc, QObject* parent) : QAbstractTableModel(parent), mRpc(nullptr) {
        setRpc(rpc);
    }

    int TorrentsModel::columnCount(const QModelIndex&) const { return QMetaEnum::fromType<Column>().keyCount(); }

    QVariant TorrentsModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        Torrent* torrent = mRpc->torrents().at(static_cast<size_t>(index.row())).get();
        switch (role) {
        case Qt::DecorationRole:
            if (static_cast<Column>(index.column()) == Column::Name) {
                using namespace desktoputils;
                if (torrent->error() != TorrentData::Error::None) {
                    return QPixmap(statusIconPath(ErroredIcon));
                }
                switch (torrent->status()) {
                case TorrentData::Status::Paused:
                    return QPixmap(statusIconPath(PausedIcon));
                case TorrentData::Status::Seeding:
                    if (torrent->isSeedingStalled()) {
                        return QPixmap(statusIconPath(StalledSeedingIcon));
                    }
                    return QPixmap(statusIconPath(SeedingIcon));
                case TorrentData::Status::Downloading:
                    if (torrent->isDownloadingStalled()) {
                        return QPixmap(statusIconPath(StalledDownloadingIcon));
                    }
                    return QPixmap(statusIconPath(DownloadingIcon));
                case TorrentData::Status::QueuedForDownloading:
                case TorrentData::Status::QueuedForSeeding:
                    return QPixmap(statusIconPath(QueuedIcon));
                case TorrentData::Status::Checking:
                case TorrentData::Status::QueuedForChecking:
                    return QPixmap(statusIconPath(CheckingIcon));
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
                if (torrent->status() == TorrentData::Status::Checking) {
                    return Utils::formatProgress(torrent->recheckProgress());
                }
                return Utils::formatProgress(torrent->percentDone());
            case Column::Status: {
                switch (torrent->status()) {
                case TorrentData::Status::Paused:
                    if (torrent->hasError()) {
                        return qApp->translate("tremotesf", "Paused (%1)", "Torrent status with error")
                            .arg(torrent->errorString());
                    }
                    return qApp->translate("tremotesf", "Paused", "Torrent status");
                case TorrentData::Status::Downloading:
                    if (torrent->hasError()) {
                        return qApp->translate("tremotesf", "Downloading (%1)", "Torrent status with error")
                            .arg(torrent->errorString());
                    }
                    return qApp->translate("tremotesf", "Downloading", "Torrent status");
                case TorrentData::Status::Seeding:
                    if (torrent->hasError()) {
                        return qApp->translate("tremotesf", "Seeding (%1)", "Torrent status with error")
                            .arg(torrent->errorString());
                    }
                    return qApp->translate("tremotesf", "Seeding", "Torrent status");
                case TorrentData::Status::QueuedForDownloading:
                case TorrentData::Status::QueuedForSeeding:
                    if (torrent->hasError()) {
                        return qApp->translate("tremotesf", "Queued (%1)", "Torrent status with error")
                            .arg(torrent->errorString());
                    }
                    return qApp->translate("tremotesf", "Queued", "Torrent status");
                case TorrentData::Status::Checking:
                    if (torrent->hasError()) {
                        return qApp->translate("tremotesf", "Checking (%1)", "Torrent status with error")
                            .arg(torrent->errorString());
                    }
                    return qApp->translate("tremotesf", "Checking", "Torrent status");
                case TorrentData::Status::QueuedForChecking:
                    if (torrent->hasError()) {
                        return qApp->translate("tremotesf", "Queued for checking (%1)", "Torrent status with error")
                            .arg(torrent->errorString());
                    }
                    return qApp->translate("tremotesf", "Queued for checking");
                }
                break;
            }
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
                return torrent->addedDate().toLocalTime();
            case Column::DoneDate:
                return torrent->doneDate().toLocalTime();
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
                return torrent->activityDate().toLocalTime();
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
                if (torrent->status() == TorrentData::Status::Checking) {
                    return torrent->recheckProgress();
                }
                return torrent->percentDone();
            case Column::Status:
                return static_cast<int>(torrent->status());
            case Column::DownloadSpeed:
                return torrent->downloadSpeed();
            case Column::UploadSpeed:
                return torrent->uploadSpeed();
            case Column::Eta: {
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

    QVariant TorrentsModel::headerData(int section, Qt::Orientation orientation, int role) const {
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

    int TorrentsModel::rowCount(const QModelIndex&) const { return static_cast<int>(mRpc->torrentsCount()); }

    Rpc* TorrentsModel::rpc() const { return mRpc; }

    void TorrentsModel::setRpc(Rpc* rpc) {
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

                QObject::connect(rpc, &Rpc::onAddedTorrents, this, [=] { endInsertRows(); });

                QObject::connect(rpc, &Rpc::onAboutToRemoveTorrents, this, [=](size_t first, size_t last) {
                    beginRemoveRows({}, static_cast<int>(first), static_cast<int>(last - 1));
                });

                QObject::connect(rpc, &Rpc::onRemovedTorrents, this, [=] { endRemoveRows(); });

                QObject::connect(rpc, &Rpc::onChangedTorrents, this, [=](size_t first, size_t last) {
                    emit dataChanged(
                        index(static_cast<int>(first), 0),
                        index(static_cast<int>(last), columnCount() - 1)
                    );
                });

                const auto count = rpc->torrentsCount();
                if (count != 0) {
                    beginInsertRows({}, 0, count - 1);
                    endInsertRows();
                }
            }
        }
    }

    Torrent* TorrentsModel::torrentAtIndex(const QModelIndex& index) const { return torrentAtRow(index.row()); }

    Torrent* TorrentsModel::torrentAtRow(int row) const { return mRpc->torrents()[static_cast<size_t>(row)].get(); }

    std::vector<int> TorrentsModel::idsFromIndexes(const QModelIndexList& indexes) const {
        std::vector<int> ids{};
        ids.reserve(static_cast<size_t>(indexes.size()));
        std::transform(indexes.begin(), indexes.end(), std::back_inserter(ids), [this](const QModelIndex& index) {
            return torrentAtIndex(index)->id();
        });
        return ids;
    }
}
