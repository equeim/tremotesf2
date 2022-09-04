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

#ifndef TREMOTESF_TORRENTFILESMODEL_H
#define TREMOTESF_TORRENTFILESMODEL_H

#include <vector>

#include "tremotesf/ui/itemmodels/basetorrentfilesmodel.h"
#include "libtremotesf/torrent.h"

namespace tremotesf
{
    class Rpc;

    class TorrentFilesModel : public BaseTorrentFilesModel
    {
        Q_OBJECT
    public:
        explicit TorrentFilesModel(libtremotesf::Torrent* torrent = nullptr,
                                   Rpc* rpc = nullptr,
                                   QObject* parent = nullptr);
        ~TorrentFilesModel() override;

        libtremotesf::Torrent* torrent() const;
        void setTorrent(libtremotesf::Torrent* torrent);

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        void setFileWanted(const QModelIndex& index, bool wanted) override;
        void setFilesWanted(const QModelIndexList& indexes, bool wanted) override;
        void setFilePriority(const QModelIndex& index, tremotesf::TorrentFilesModelEntry::Priority priority) override;
        void setFilesPriority(const QModelIndexList& indexes, tremotesf::TorrentFilesModelEntry::Priority priority) override;

        void renameFile(const QModelIndex& index, const QString& newName) override;
        void fileRenamed(const QString& path, const QString& newName);

        QString localFilePath(const QModelIndex& index) const;
        bool isWanted(const QModelIndex& index) const;

    private:
        void update(const std::vector<int>& changed);
        void createTree();
        void resetTree();
        void updateTree(const std::vector<int>& changed);

        void setLoaded(bool loaded);

        libtremotesf::Torrent* mTorrent{};
        Rpc* mRpc{};
        std::vector<TorrentFilesModelFile*> mFiles{};
        bool mCreatingTree{};
        bool mLoaded{};
    };
}

#endif // TREMOTESF_TORRENTFILESMODEL_H
