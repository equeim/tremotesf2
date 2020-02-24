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

#ifndef TREMOTESF_BASETORRENTFILESMODEL_H
#define TREMOTESF_BASETORRENTFILESMODEL_H

#include <memory>
#include <vector>
#include <QAbstractItemModel>

#include "torrentfilesmodelentry.h"

namespace tremotesf
{
    class BaseTorrentFilesModel : public QAbstractItemModel
    {
        Q_OBJECT
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
        Q_ENUM(Role)

        explicit BaseTorrentFilesModel(QObject* parent = nullptr);
#else
        enum Column
        {
            NameColumn,
            SizeColumn,
            ProgressBarColumn,
            ProgressColumn,
            PriorityColumn
        };
        static const int SortRole = Qt::UserRole;

        explicit BaseTorrentFilesModel(std::vector<Column>&& columns, QObject* parent = nullptr);
#endif

        int columnCount(const QModelIndex& = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
#ifndef TREMOTESF_SAILFISHOS
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;
#endif

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent) const override;

        Q_INVOKABLE virtual void setFileWanted(const QModelIndex& index, bool wanted);
        Q_INVOKABLE virtual void setFilesWanted(const QModelIndexList& indexes, bool wanted);
        Q_INVOKABLE virtual void setFilePriority(const QModelIndex& index, tremotesf::TorrentFilesModelEntry::Priority priority);
        Q_INVOKABLE virtual void setFilesPriority(const QModelIndexList& indexes, tremotesf::TorrentFilesModelEntry::Priority priority);

        Q_INVOKABLE virtual void renameFile(const QModelIndex& index, const QString& newName) = 0;
        void fileRenamed(TorrentFilesModelEntry* entry, const QString& newName);

    protected:
        void updateDirectoryChildren(const QModelIndex& parent = QModelIndex());

        std::shared_ptr<TorrentFilesModelDirectory> mRootDirectory;

    private:
#ifndef TREMOTESF_SAILFISHOS
        const std::vector<Column> mColumns;
#endif
    };
}

#endif // TREMOTESF_BASETORRENTFILESMODEL_H
