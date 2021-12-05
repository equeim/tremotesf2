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

#include "basetorrentfilesmodel.h"

#ifndef TREMOTESF_SAILFISHOS
#include <QApplication>
#include <QStyle>
#endif

#include "libtremotesf/itemlistupdater.h"
#include "utils.h"

namespace tremotesf
{
#ifdef TREMOTESF_SAILFISHOS
    BaseTorrentFilesModel::BaseTorrentFilesModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }
#else
    BaseTorrentFilesModel::BaseTorrentFilesModel(std::vector<Column>&& columns, QObject* parent)
        : QAbstractItemModel(parent),
          mColumns(std::move(columns))
    {
    }
#endif

    int BaseTorrentFilesModel::columnCount(const QModelIndex&) const
    {
#ifdef TREMOTESF_SAILFISHOS
        return 1;
#else
        return static_cast<int>(mColumns.size());
#endif
    }

    QVariant BaseTorrentFilesModel::data(const QModelIndex& index, int role) const
    {
        const TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
#ifdef TREMOTESF_SAILFISHOS
        switch (role) {
        case NameRole:
            return entry->name();
        case IsDirectoryRole:
            return entry->isDirectory();
        case CompletedSizeRole:
            return entry->completedSize();
        case SizeRole:
            return entry->size();
        case ProgressRole:
            return entry->progress();
        case WantedStateRole:
            return entry->wantedState();
        case PriorityRole:
            return entry->priority();
        }
#else
        const Column column = mColumns[static_cast<size_t>(index.column())];
        switch (role) {
        case Qt::CheckStateRole:
            if (column == NameColumn) {
                switch (entry->wantedState()) {
                case TorrentFilesModelEntry::Wanted:
                    return Qt::Checked;
                case TorrentFilesModelEntry::Unwanted:
                    return Qt::Unchecked;
                case TorrentFilesModelEntry::MixedWanted:
                    return Qt::PartiallyChecked;
                }
            }
            break;
        case Qt::DecorationRole:
            if (column == NameColumn) {
                if (entry->isDirectory()) {
                    return qApp->style()->standardIcon(QStyle::SP_DirIcon);
                }
                return qApp->style()->standardIcon(QStyle::SP_FileIcon);
            }
            break;
        case Qt::DisplayRole:
            switch (column) {
            case NameColumn:
                return entry->name();
            case SizeColumn:
                return Utils::formatByteSize(entry->size());
            case ProgressColumn:
                return Utils::formatProgress(entry->progress());
            case PriorityColumn:
                return entry->priorityString();
            default:
                break;
            }
            break;
        case Qt::ToolTipRole:
            if (index.column() == NameColumn) {
                return entry->name();
            }
            break;
        case SortRole:
            switch (column) {
            case SizeColumn:
                return entry->size();
            case ProgressBarColumn:
            case ProgressColumn:
                return entry->progress();
            case PriorityColumn:
                return entry->priority();
            default:
                return data(index, Qt::DisplayRole);
            }
        }
#endif
        return QVariant();
    }

#ifndef TREMOTESF_SAILFISHOS
    Qt::ItemFlags BaseTorrentFilesModel::flags(const QModelIndex& index) const
    {
        if (index.column() == NameColumn) {
            return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
        }
        return QAbstractItemModel::flags(index);
    }

    QVariant BaseTorrentFilesModel::headerData(int section, Qt::Orientation, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (mColumns[static_cast<size_t>(section)]) {
            case NameColumn:
                return qApp->translate("tremotesf", "Name");
            case SizeColumn:
                return qApp->translate("tremotesf", "Size");
            case ProgressBarColumn:
                return qApp->translate("tremotesf", "Progress Bar");
            case ProgressColumn:
                return qApp->translate("tremotesf", "Progress");
            case PriorityColumn:
                return qApp->translate("tremotesf", "Priority");
            default:
                break;
            }
        }
        return QVariant();
    }

    bool BaseTorrentFilesModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (index.column() == NameColumn && role == Qt::CheckStateRole) {
            setFileWanted(index, (value.toInt() == Qt::Checked));
            return true;
        }
        return false;
    }
#endif

    QModelIndex BaseTorrentFilesModel::index(int row, int column, const QModelIndex& parent) const
    {
        const TorrentFilesModelDirectory* parentDirectory;
        if (parent.isValid()) {
            parentDirectory = static_cast<TorrentFilesModelDirectory*>(parent.internalPointer());
        } else if (mRootDirectory) {
            parentDirectory = mRootDirectory.get();
        } else {
            return {};
        }
        if (row >= 0 && static_cast<size_t>(row) < parentDirectory->children().size()) {
            return createIndex(row, column, parentDirectory->children()[static_cast<size_t>(row)].get());
        }
        return {};
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
                return static_cast<int>(static_cast<const TorrentFilesModelDirectory*>(entry)->children().size());
            }
            return 0;
        }
        if (mRootDirectory) {
            return static_cast<int>(mRootDirectory->children().size());
        }
        return 0;
    }

    void BaseTorrentFilesModel::setFileWanted(const QModelIndex& index, bool wanted)
    {
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        updateDirectoryChildren();
    }

    void BaseTorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
        }
        updateDirectoryChildren();
    }

    void BaseTorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntry::Priority priority)
    {
        static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        updateDirectoryChildren();
    }

    void BaseTorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority)
    {
        for (const QModelIndex& index : indexes) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
        }
        updateDirectoryChildren();
    }

    void BaseTorrentFilesModel::fileRenamed(TorrentFilesModelEntry* entry, const QString& newName)
    {
        entry->setName(newName);
        emit dataChanged(createIndex(entry->row(), 0, entry),
                         createIndex(entry->row(), columnCount() - 1, entry));
    }

    void BaseTorrentFilesModel::updateDirectoryChildren(const QModelIndex& parent)
    {
        const TorrentFilesModelDirectory* directory;
        if (parent.isValid()) {
            directory = static_cast<const TorrentFilesModelDirectory*>(parent.internalPointer());
        } else if (mRootDirectory) {
            directory = mRootDirectory.get();
        } else {
            return;
        }

        auto changedBatchProcessor = ItemBatchProcessor([&](size_t first, size_t last) {
            emit dataChanged(index(static_cast<int>(first), 0, parent), index(static_cast<int>(last) - 1, columnCount() - 1, parent));
        });

        for (auto& child : directory->children()) {
            if (child->isChanged()) {
                changedBatchProcessor.nextIndex(static_cast<size_t>(child->row()));
                if (child->isDirectory()) {
                    updateDirectoryChildren(index(child->row(), 0, parent));
                } else {
                    static_cast<TorrentFilesModelFile*>(child.get())->setChanged(false);
                }
            }
        }
        changedBatchProcessor.commitIfNeeded();
    }
}
