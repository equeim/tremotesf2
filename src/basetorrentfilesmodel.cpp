/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include "basetorrentfilesmodel.h"

namespace tremotesf
{
    BaseTorrentFilesModel::BaseTorrentFilesModel(QObject* parent)
        : QAbstractItemModel(parent),
          mRootDirectory(new TorrentFilesModelDirectory())
    {

    }

    QModelIndex BaseTorrentFilesModel::index(int row, int column, const QModelIndex& parent) const
    {
        const TorrentFilesModelDirectory* parentDirectory;
        if (parent.isValid()) {
            parentDirectory = static_cast<TorrentFilesModelDirectory*>(parent.internalPointer());
        } else {
            parentDirectory = mRootDirectory.get();
        }

        if (row >= 0 && row < parentDirectory->children().size()) {
            return createIndex(row, column, parentDirectory->children().at(row));
        }
        return QModelIndex();
    }

    QModelIndex BaseTorrentFilesModel::parent(const QModelIndex& child) const
    {
        if (!child.isValid()) {
            return QModelIndex();
        }

        TorrentFilesModelDirectory* parentDirectory = static_cast<TorrentFilesModelEntry*>(child.internalPointer())->parentDirectory();
        if (parentDirectory == mRootDirectory.get()) {
            return QModelIndex();
        }

        return createIndex(parentDirectory->row(), 0, parentDirectory);
    }

    int BaseTorrentFilesModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) {
            const TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelDirectory*>(parent.internalPointer());
            if (entry->isDirectory()) {
                return static_cast<const TorrentFilesModelDirectory*>(entry)->children().size();
            }
            return 0;
        }
        return mRootDirectory->children().size();
    }

    void BaseTorrentFilesModel::updateDirectoryChildren(const TorrentFilesModelDirectory* directory)
    {
        const int lastColumn = columnCount() - 1;
        for (TorrentFilesModelEntry* child : directory->children()) {
            if (child->isChanged()) {
                emit dataChanged(createIndex(child->row(), 0, child),
                                 createIndex(child->row(), lastColumn, child));
                if (child->isDirectory()) {
                    updateDirectoryChildren(static_cast<const TorrentFilesModelDirectory*>(child));
                }
            }
        }
    }
}
