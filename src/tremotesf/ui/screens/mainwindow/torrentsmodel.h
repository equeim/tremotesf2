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

#ifndef TREMOTESF_TORRENTSMODEL_H
#define TREMOTESF_TORRENTSMODEL_H

#include <memory>
#include <vector>

#include <QAbstractTableModel>

namespace libtremotesf
{
    class Torrent;
}

namespace tremotesf
{
    class Rpc;

    class TorrentsModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        enum class Column
        {
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

        enum class Role
        {
            Sort = Qt::UserRole,
            TextElideMode
        };

        explicit TorrentsModel(Rpc* rpc = nullptr, QObject* parent = nullptr);

        int columnCount(const QModelIndex& = {}) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        libtremotesf::Torrent* torrentAtIndex(const QModelIndex& index) const;
        libtremotesf::Torrent* torrentAtRow(int row) const;

        QVariantList idsFromIndexes(const QModelIndexList& indexes) const;

    private:
        Rpc* mRpc;
    };
}

#endif // TREMOTESF_TORRENTSMODEL_H
