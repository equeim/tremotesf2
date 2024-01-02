// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "peersmodel.h"

#include <QCoreApplication>
#include <QMetaEnum>

#include "log/log.h"
#include "rpc/torrent.h"
#include "formatutils.h"

namespace tremotesf {
    PeersModel::PeersModel(Torrent* torrent, QObject* parent) : QAbstractTableModel(parent) { setTorrent(torrent); }

    PeersModel::~PeersModel() {
        if (mTorrent) {
            mTorrent->setPeersEnabled(false);
        }
    }

    int PeersModel::columnCount(const QModelIndex&) const { return QMetaEnum::fromType<Column>().keyCount(); }

    QVariant PeersModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const Peer& peer = mPeers.at(static_cast<size_t>(index.row()));
        switch (role) {
        case Qt::DisplayRole:
            switch (static_cast<Column>(index.column())) {
            case Column::Address:
                return peer.address;
            case Column::DownloadSpeed:
                return formatutils::formatByteSpeed(peer.downloadSpeed);
            case Column::UploadSpeed:
                return formatutils::formatByteSpeed(peer.uploadSpeed);
            case Column::ProgressBar:
            case Column::Progress:
                return formatutils::formatProgress(peer.progress);
            case Column::Flags:
                return peer.flags;
            case Column::Client:
                return peer.client;
            default:
                break;
            }
            break;
        case Qt::ToolTipRole:
            switch (static_cast<Column>(index.column())) {
            case Column::Address:
            case Column::Client:
                return data(index, Qt::DisplayRole);
            default:
                break;
            }
            break;
        case SortRole:
            switch (static_cast<Column>(index.column())) {
            case Column::DownloadSpeed:
                return peer.downloadSpeed;
            case Column::UploadSpeed:
                return peer.uploadSpeed;
            case Column::ProgressBar:
            case Column::Progress:
                return peer.progress;
            default:
                return data(index, Qt::DisplayRole);
            }
        default:
            break;
        }
        return {};
    }

    QVariant PeersModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (static_cast<Column>(section)) {
        case Column::Address:
            //: Peers list column title
            return qApp->translate("tremotesf", "Address");
        case Column::DownloadSpeed:
            //: Peers list column title
            return qApp->translate("tremotesf", "Down Speed");
        case Column::UploadSpeed:
            //: Peers list column title
            return qApp->translate("tremotesf", "Up Speed");
        case Column::ProgressBar:
            //: Peers list column title
            return qApp->translate("tremotesf", "Progress Bar");
        case Column::Progress:
            //: Peers list column title
            return qApp->translate("tremotesf", "Progress");
        case Column::Flags:
            //: Peers list column title
            return qApp->translate("tremotesf", "Flags");
        case Column::Client:
            //: Peers list column title
            return qApp->translate("tremotesf", "Client");
        default:
            return {};
        }
    }

    int PeersModel::rowCount(const QModelIndex&) const { return static_cast<int>(mPeers.size()); }

    bool PeersModel::removeRows(int row, int count, const QModelIndex& parent) {
        beginRemoveRows(parent, row, row + count - 1);
        const auto first(mPeers.begin() + row);
        mPeers.erase(first, first + count);
        endRemoveRows();
        return true;
    }

    Torrent* PeersModel::torrent() const { return mTorrent; }

    void PeersModel::setTorrent(Torrent* torrent) {
        if (torrent != mTorrent) {
            if (const auto oldTorrent = mTorrent.data(); oldTorrent) {
                QObject::disconnect(oldTorrent, nullptr, this, nullptr);
            }

            mTorrent = torrent;

            if (mTorrent) {
                QObject::connect(mTorrent, &Torrent::peersUpdated, this, &PeersModel::update);
                if (mTorrent->isPeersEnabled()) {
                    logWarning("{} already has enabled peers, this shouldn't happen", *mTorrent);
                }
                mTorrent->setPeersEnabled(true);
            } else {
                beginResetModel();
                mPeers.clear();
                endResetModel();
            }
        }
    }

    void PeersModel::update(
        const std::vector<std::pair<int, int>>& removedIndexRanges,
        const std::vector<std::pair<int, int>>& changedIndexRanges,
        int addedCount
    ) {
        for (const auto& [first, last] : removedIndexRanges) {
            beginRemoveRows({}, first, last - 1);
            mPeers.erase(mPeers.begin() + first, mPeers.begin() + last);
            endRemoveRows();
        }

        const auto& newPeers = mTorrent->peers();

        for (const auto& [first, last] : changedIndexRanges) {
            std::copy(newPeers.begin() + first, newPeers.begin() + last, mPeers.begin() + first);
            emit dataChanged(index(0, columnCount() - 1), index(last - 1, columnCount() - 1));
        }

        if (addedCount > 0) {
            beginInsertRows(QModelIndex(), static_cast<int>(mPeers.size()), static_cast<int>(newPeers.size()) - 1);
            mPeers.reserve(newPeers.size());
            std::copy(
                newPeers.begin() + static_cast<ptrdiff_t>(mPeers.size()),
                newPeers.end(),
                std::back_insert_iterator(mPeers)
            );
            endInsertRows();
        }
    }
}
