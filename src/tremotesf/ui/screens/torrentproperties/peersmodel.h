// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_PEERSMODEL_H
#define TREMOTESF_PEERSMODEL_H

#include <vector>

#include <QAbstractTableModel>

#include "libtremotesf/peer.h"

namespace libtremotesf {
    class Torrent;
}

namespace tremotesf {
    class PeersModel : public QAbstractTableModel {
        Q_OBJECT
    public:
        enum class Column { Address, DownloadSpeed, UploadSpeed, ProgressBar, Progress, Flags, Client };
        Q_ENUM(Column)

        static constexpr auto SortRole = Qt::UserRole;

        explicit PeersModel(libtremotesf::Torrent* torrent = nullptr, QObject* parent = nullptr);
        ~PeersModel() override;

        int columnCount(const QModelIndex& = {}) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex&) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        libtremotesf::Torrent* torrent() const;
        void setTorrent(libtremotesf::Torrent* torrent);

    private:
        void update(
            const std::vector<std::pair<int, int>>& removedIndexRanges,
            const std::vector<std::pair<int, int>>& changedIndexRanges,
            int addedCount
        );

        std::vector<libtremotesf::Peer> mPeers{};
        libtremotesf::Torrent* mTorrent{};
    };
}

#endif // TREMOTESF_PEERSMODEL_H
