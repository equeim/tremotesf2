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

#include "basetorrentfilesmodel.h"
#include "libtremotesf/torrent.h"

namespace tremotesf
{
    class TorrentFilesModel : public BaseTorrentFilesModel
    {
        Q_OBJECT
        Q_ENUMS(Role)
        Q_PROPERTY(libtremotesf::Torrent* torrent READ torrent WRITE setTorrent)
        Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadedChanged)
        Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    public:
#ifdef TREMOTESF_SAILFISHOS
        enum Role
        {
            NameRole = Qt::UserRole,
            IsDirectoryRole,
            CompletedSizeRole,
            SizeRole,
            ProgressRole,
            WantedStateRole,
            PriorityRole
        };
#else
        enum Column
        {
            NameColumn,
            SizeColumn,
            ProgressBarColumn,
            ProgressColumn,
            PriorityColumn,
            ColumnCount
        };
#endif

        explicit TorrentFilesModel(libtremotesf::Torrent* torrent = nullptr, QObject* parent = nullptr);
        ~TorrentFilesModel() override;

        int columnCount(const QModelIndex& = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
#ifndef TREMOTESF_SAILFISHOS
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;
#endif

        libtremotesf::Torrent* torrent() const;
        void setTorrent(libtremotesf::Torrent* torrent);

        bool isLoaded() const;
        bool isLoading() const;

        Q_INVOKABLE void setFileWanted(const QModelIndex& index, bool wanted) override;
        Q_INVOKABLE void setFilesWanted(const QModelIndexList& indexes, bool wanted) override;
        Q_INVOKABLE void setFilePriority(const QModelIndex& index, tremotesf::TorrentFilesModelEntry::Priority priority) override;
        Q_INVOKABLE void setFilesPriority(const QModelIndexList& indexes, tremotesf::TorrentFilesModelEntry::Priority priority) override;

        Q_INVOKABLE void renameFile(const QModelIndex& index, const QString& newName) const;
        void fileRenamed(const QString& path, const QString& newName);

#ifdef TREMOTESF_SAILFISHOS
    protected:
        QHash<int, QByteArray> roleNames() const override;
#endif

    private:
        void update(const std::vector<std::shared_ptr<libtremotesf::TorrentFile>>& files);
        void createTree(const std::vector<std::shared_ptr<libtremotesf::TorrentFile>>& files);
        void resetTree();
        void updateTree(const std::vector<std::shared_ptr<libtremotesf::TorrentFile>>& files, bool emitSignal);

        void setLoaded(bool loaded);
        void setLoading(bool loading);

        libtremotesf::Torrent* mTorrent;
        bool mLoaded;
        bool mLoading;
        std::vector<TorrentFilesModelFile*> mFiles;
        bool mCreatingTree;
        bool mResetAfterCreate;
        bool mUpdateAfterCreate;
    signals:
        void loadedChanged();
        void loadingChanged();
    };
}

#endif // TREMOTESF_TORRENTFILESMODEL_H
