// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_PEERSMODEL_H
#define TREMOTESF_PEERSMODEL_H

#include <vector>

#include <QAbstractTableModel>

#include "rpc/peer.h"

namespace tremotesf {
    class Torrent;
}

namespace tremotesf {
    class PeersModel final : public QAbstractTableModel {
        Q_OBJECT

    public:
        enum class Column { Address, DownloadSpeed, UploadSpeed, ProgressBar, Progress, Flags, Client };
        Q_ENUM(Column)

        static constexpr auto SortRole = Qt::UserRole;

        inline explicit PeersModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
        ~PeersModel() override;
        Q_DISABLE_COPY_MOVE(PeersModel)

        int columnCount(const QModelIndex& = {}) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex&) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        Torrent* torrent() const;
        void setTorrent(Torrent* torrent);

    private:
        void setTorrent(Torrent* torrent, bool oldTorrentDestroyed);

        void update(
            const std::vector<std::pair<int, int>>& removedIndexRanges,
            const std::vector<std::pair<int, int>>& changedIndexRanges,
            int addedCount
        );

        std::vector<Peer> mPeers{};
        Torrent* mTorrent{};
    };
}

#endif // TREMOTESF_PEERSMODEL_H
