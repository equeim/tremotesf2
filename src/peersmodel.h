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

#ifndef TREMOTESF_PEERSMODEL_H
#define TREMOTESF_PEERSMODEL_H

#include <vector>

#include <QAbstractTableModel>

#include "libtremotesf/peer.h"

namespace libtremotesf
{
    class Torrent;
}

namespace tremotesf
{
    class PeersModel : public QAbstractTableModel
    {
        Q_OBJECT
        Q_PROPERTY(libtremotesf::Torrent* torrent READ torrent WRITE setTorrent)
        Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
    public:
#ifdef TREMOTESF_SAILFISHOS
        enum Role
        {
            Address = Qt::UserRole,
            DownloadSpeed,
            UploadSpeed,
            Progress,
            Flags,
            Client
        };
        Q_ENUMS(Role)
#else
        enum Column
        {
            AddressColumn,
            DownloadSpeedColumn,
            UploadSpeedColumn,
            ProgressBarColumn,
            ProgressColumn,
            FlagsColumn,
            ClientColumn,
            ColumnCount
        };
        static const int SortRole;
#endif

        explicit PeersModel(libtremotesf::Torrent* torrent = nullptr, QObject* parent = nullptr);
        ~PeersModel() override;

        int columnCount(const QModelIndex& = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
#ifndef TREMOTESF_SAILFISHOS
        QVariant headerData(int section, Qt::Orientation, int role) const override;
#endif
        int rowCount(const QModelIndex&) const override;

        libtremotesf::Torrent* torrent() const;
        void setTorrent(libtremotesf::Torrent* torrent);

        bool isLoaded() const;

#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif
    private:
        void update(const std::vector<int>& removed, const std::vector<int>& changed, int added);

        std::vector<libtremotesf::Peer> mPeers;
        libtremotesf::Torrent* mTorrent;
        bool mLoaded;
    signals:
        void loadedChanged();
    };
}

#endif // TREMOTESF_PEERSMODEL_H
