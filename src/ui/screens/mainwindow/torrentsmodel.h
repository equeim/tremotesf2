// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTSMODEL_H
#define TREMOTESF_TORRENTSMODEL_H

#include <vector>

#include <QAbstractTableModel>
#include <QPointer>

namespace tremotesf {
    class Torrent;
}

namespace tremotesf {
    class Rpc;

    class TorrentsModel final : public QAbstractTableModel {
        Q_OBJECT

    public:
        enum class Column {
            Name,
            SizeWhenDone,
            TotalSize,
            ProgressBar,
            Progress,
            Status,
            Priority,
            QueuePosition,
            Seeders,
            Leechers,
            PeersSendingToUs,
            PeersGettingFromUs,
            DownloadSpeed,
            UploadSpeed,
            Eta,
            Ratio,
            AddedDate,
            DoneDate,
            DownloadSpeedLimit,
            UploadSpeedLimit,
            TotalDownloaded,
            TotalUploaded,
            LeftUntilDone,
            DownloadDirectory,
            CompletedSize,
            ActivityDate
        };
        Q_ENUM(Column)

        enum class Role { Sort = Qt::UserRole };

        explicit TorrentsModel(Rpc* rpc = nullptr, QObject* parent = nullptr);

        int columnCount(const QModelIndex& = {}) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        Torrent* torrentAtIndex(const QModelIndex& index) const;
        Torrent* torrentAtRow(int row) const;

        std::vector<int> idsFromIndexes(const QModelIndexList& indexes) const;

    private:
        QPointer<Rpc> mRpc;
        bool mUseRelativeTime{};
        bool mDisplayFullDownloadDirectoryPath{};
    };
}

#endif // TREMOTESF_TORRENTSMODEL_H
