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

#include "peersmodel.h"

#include <QCoreApplication>

#include "torrent.h"
#include "utils.h"

namespace tremotesf
{
    namespace
    {
        const QString addressKey(QLatin1String("address"));
        const QString downloadSpeedKey(QLatin1String("rateToClient"));
        const QString uploadSpeedKey(QLatin1String("rateToPeer"));
        const QString progressKey(QLatin1String("progress"));
        const QString flagsKey(QLatin1String("flagStr"));
        const QString clientKey(QLatin1String("clientName"));
    }

    Peer::Peer(const QString& address, const QVariantMap& peerMap)
        : address(address)
    {
        update(peerMap);
    }

    void Peer::update(const QVariantMap& peerMap)
    {
        downloadSpeed = peerMap.value(downloadSpeedKey).toLongLong();
        uploadSpeed = peerMap.value(uploadSpeedKey).toLongLong();
        progress = peerMap.value(progressKey).toFloat();
        flags = peerMap.value(flagsKey).toString();
        client = peerMap.value(clientKey).toString();
    }

#ifndef TREMOTESF_SAILFISHOS
    const int PeersModel::SortRole = Qt::UserRole;
#endif

    PeersModel::PeersModel(Torrent* torrent, QObject* parent)
        : QAbstractTableModel(parent),
          mTorrent(nullptr),
          mLoaded(false)
    {
        setTorrent(torrent);
    }

    PeersModel::~PeersModel()
    {
        if (mTorrent) {
            mTorrent->setPeersEnabled(false);
        }
    }

    int PeersModel::columnCount(const QModelIndex&) const
    {
#ifdef TREMOTESF_SAILFISHOS
        return 1;
#else
        return ColumnCount;
#endif
    }

    QVariant PeersModel::data(const QModelIndex& index, int role) const
    {
        const Peer* peer = mPeers[index.row()].get();
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case Address:
            return peer->address;
        case DownloadSpeed:
            return peer->downloadSpeed;
        case UploadSpeed:
            return peer->uploadSpeed;
        case Progress:
            return peer->progress;
        case Flags:
            return peer->flags;
        case Client:
            return peer->client;
        }
#else
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case AddressColumn:
                return peer->address;
            case DownloadSpeedColumn:
                return Utils::formatByteSpeed(peer->downloadSpeed);
            case UploadSpeedColumn:
                return Utils::formatByteSpeed(peer->uploadSpeed);
            case ProgressColumn:
                return Utils::formatProgress(peer->progress);
            case FlagsColumn:
                return peer->flags;
            case ClientColumn:
                return peer->client;
            }
            break;
        case SortRole:
            switch (index.column()) {
            case DownloadSpeedColumn:
                return peer->downloadSpeed;
            case UploadSpeedColumn:
                return peer->uploadSpeed;
            case ProgressBarColumn:
            case ProgressColumn:
                return peer->progress;
            default:
                return data(index, Qt::DisplayRole);
            }
        }
#endif
        return QVariant();
    }

#ifndef TREMOTESF_SAILFISHOS
    QVariant PeersModel::headerData(int section, Qt::Orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case AddressColumn:
                return qApp->translate("tremotesf", "Address");
            case DownloadSpeedColumn:
                return qApp->translate("tremotesf", "Down Speed");
            case UploadSpeedColumn:
                return qApp->translate("tremotesf", "Up Speed");
            case ProgressBarColumn:
                return qApp->translate("tremotesf", "Progress Bar");
            case ProgressColumn:
                return qApp->translate("tremotesf", "Progress");
            case FlagsColumn:
                return qApp->translate("tremotesf", "Flags");
            case ClientColumn:
                return qApp->translate("tremotesf", "Client");
            }
        }
        return QVariant();
    }
#endif

    int PeersModel::rowCount(const QModelIndex&) const
    {
        return mPeers.size();
    }

    Torrent* PeersModel::torrent() const
    {
        return mTorrent;
    }

    void PeersModel::setTorrent(Torrent* torrent)
    {
        if (!torrent || mTorrent) {
            return;
        }

        if (torrent != mTorrent) {
            mTorrent = torrent;

            if (mTorrent) {
                QObject::connect(mTorrent, &Torrent::peersUpdated, this, [=](const QVariantList& peers) {
                    QStringList addresses;
                    for (const QVariant& peer : peers) {
                        addresses.append(peer.toMap().value(addressKey).toString());
                    }

                    for (int i = 0, max = mPeers.size(); i < max; ++i) {
                        if (!addresses.contains(mPeers[i]->address)) {
                            beginRemoveRows(QModelIndex(), i, i);
                            mPeers.erase(mPeers.begin() + i);
                            endRemoveRows();
                            i--;
                            max--;
                        }
                    }

                    for (const QVariant& peerVariant : peers) {
                        const QVariantMap peerMap(peerVariant.toMap());
                        const QString address(peerMap.value(addressKey).toString());
                        int row = -1;
                        for (int i = 0, max = mPeers.size(); i < max; ++i) {
                            if (mPeers[i]->address == address) {
                                row = i;
                                break;
                            }
                        }
                        if (row == -1) {
                            row = mPeers.size();
                            beginInsertRows(QModelIndex(), row, row);
                            mPeers.push_back(std::make_shared<Peer>(address, peerMap));
                            endInsertRows();
                        } else {
                            mPeers[row]->update(peerMap);
                            emit dataChanged(index(row, 0), index(row, columnCount() - 1));
                        }
                    }

                    if (!mLoaded) {
                        mLoaded = true;
                        emit loadedChanged();
                    }
                });

                mTorrent->setPeersEnabled(true);
            } else {
                beginResetModel();
                mPeers.clear();
                endResetModel();
            }
        }
    }

    bool PeersModel::isLoaded() const
    {
        return mLoaded;
    }

#ifdef TREMOTESF_SAILFISHOS
    QHash<int, QByteArray> PeersModel::roleNames() const
    {
        return {{Address, "address"},
                {DownloadSpeed, "downloadSpeed"},
                {UploadSpeed, "uploadSpeed"},
                {Progress, "progress"},
                {Flags, "flags"},
                {Client, "client"}};
    }
#endif
}
