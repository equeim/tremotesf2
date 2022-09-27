// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
