// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESMODEL_H
#define TREMOTESF_TORRENTFILESMODEL_H

#include <span>
#include <vector>

#include <QPointer>

#include "coroutines/scope.h"
#include "ui/itemmodels/basetorrentfilesmodel.h"
#include "rpc/torrent.h"

namespace tremotesf {
    class Rpc;

    class TorrentFilesModel final : public BaseTorrentFilesModel {
        Q_OBJECT

    public:
        explicit TorrentFilesModel(Torrent* torrent = nullptr, Rpc* rpc = nullptr, QObject* parent = nullptr);
        ~TorrentFilesModel() override;
        Q_DISABLE_COPY_MOVE(TorrentFilesModel)

        Torrent* torrent() const;
        void setTorrent(Torrent* torrent);

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        void setFileWanted(const QModelIndex& index, bool wanted) override;
        void setFilesWanted(const QModelIndexList& indexes, bool wanted) override;
        void setFilePriority(const QModelIndex& index, TorrentFilesModelEntry::Priority priority) override;
        void setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority) override;

        void renameFile(const QModelIndex& index, const QString& newName) override;
        void fileRenamed(const QString& path, const QString& newName);

        QString localFilePath(const QModelIndex& index) const;
        bool isWanted(const QModelIndex& index) const;

    private:
        void update(std::span<const int> changed);
        Coroutine<> createTree();
        void resetTree();
        void updateTree(std::span<const int> changed);

        void setLoaded(bool loaded);

        QPointer<Torrent> mTorrent{};
        Rpc* mRpc{};
        std::vector<TorrentFilesModelFile*> mFiles{};
        bool mCreatingTree{};
        bool mLoaded{};
        CoroutineScope mCoroutineScope{};
    };
}

#endif // TREMOTESF_TORRENTFILESMODEL_H
