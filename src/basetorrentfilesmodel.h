/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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
#include <QAbstractItemModel>

#include "torrentfilesmodelentry.h"

namespace tremotesf
{
    class BaseTorrentFilesModel : public QAbstractItemModel
    {
    public:
#ifndef TREMOTESF_SAILFISHOS
        static const int SortRole = Qt::UserRole;
#endif

        explicit BaseTorrentFilesModel(QObject* parent = nullptr);

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent) const override;

        virtual void setFileWanted(const QModelIndex& index, bool wanted) = 0;
        virtual void setFilesWanted(const QModelIndexList& indexes, bool wanted) = 0;
        virtual void setFilePriority(const QModelIndex& index, TorrentFilesModelEntryEnums::Priority priority) = 0;
        virtual void setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntryEnums::Priority priority) = 0;

    protected:
        void updateDirectoryChildren(const TorrentFilesModelDirectory* directory);

        std::shared_ptr<TorrentFilesModelDirectory> mRootDirectory;
    };
}

#endif // TREMOTESF_BASETORRENTFILESMODEL_H
