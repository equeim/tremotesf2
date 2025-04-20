// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "basetorrentfilesmodel.h"

#include <QCoreApplication>
#include <QIcon>

#include "desktoputils.h"
#include "itemlistupdater.h"
#include "formatutils.h"

namespace tremotesf {
    BaseTorrentFilesModel::BaseTorrentFilesModel(std::vector<Column>&& columns, QObject* parent)
        : QAbstractItemModel(parent), mColumns(std::move(columns)) {}

    int BaseTorrentFilesModel::columnCount(const QModelIndex&) const { return static_cast<int>(mColumns.size()); }

    QVariant BaseTorrentFilesModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const TorrentFilesModelEntry* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
        const Column column = mColumns.at(static_cast<size_t>(index.column()));
        switch (role) {
        case Qt::CheckStateRole:
            if (column == Column::Name) {
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
            if (column == Column::Name) {
                if (entry->isDirectory()) {
                    return desktoputils::standardDirIcon();
                }
                return desktoputils::standardFileIcon();
            }
            break;
        case Qt::DisplayRole:
            switch (column) {
            case Column::Name:
                return entry->name();
            case Column::Size:
                return formatutils::formatByteSize(entry->size());
            case Column::ProgressBar:
            case Column::Progress:
                return formatutils::formatProgress(entry->progress());
            case Column::Priority:
                return entry->priorityString();
            default:
                break;
            }
            break;
        case Qt::ToolTipRole:
            if (column == Column::Name) {
                return entry->name();
            }
            break;
        case SortRole:
            switch (column) {
            case Column::Size:
                return entry->size();
            case Column::ProgressBar:
            case Column::Progress:
                return entry->progress();
            case Column::Priority:
                return entry->priority();
            default:
                return data(index, Qt::DisplayRole);
            }
        default:
            return {};
        }
        return {};
    }

    Qt::ItemFlags BaseTorrentFilesModel::flags(const QModelIndex& index) const {
        if (!index.isValid()) {
            return {};
        }
        if (static_cast<Column>(index.column()) == Column::Name) {
            return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
        }
        return QAbstractItemModel::flags(index);
    }

    QVariant BaseTorrentFilesModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (mColumns.at(static_cast<size_t>(section))) {
        case Column::Name:
            //: Column title in torrent's file list
            return qApp->translate("tremotesf", "Name");
        case Column::Size:
            //: Column title in torrent's file list
            return qApp->translate("tremotesf", "Size");
        case Column::ProgressBar:
            //: Column title in torrent's file list
            return qApp->translate("tremotesf", "Progress Bar");
        case Column::Progress:
            //: Column title in torrent's file list
            return qApp->translate("tremotesf", "Progress");
        case Column::Priority:
            //: Column title in torrent's file list
            return qApp->translate("tremotesf", "Priority");
        default:
            return {};
        }
    }

    bool BaseTorrentFilesModel::setData(const QModelIndex& index, const QVariant& value, int role) {
        if (!index.isValid()) {
            return false;
        }
        if (static_cast<Column>(index.column()) == Column::Name && role == Qt::CheckStateRole) {
            setFileWanted(index, (value.toInt() == Qt::Checked));
            return true;
        }
        return false;
    }

    QModelIndex BaseTorrentFilesModel::index(int row, int column, const QModelIndex& parent) const {
        const TorrentFilesModelDirectory* parentDirectory{};
        if (parent.isValid()) {
            parentDirectory = static_cast<TorrentFilesModelDirectory*>(parent.internalPointer());
        } else if (mRootDirectory) {
            parentDirectory = mRootDirectory.get();
        } else {
            return {};
        }
        return createIndex(row, column, parentDirectory->children().at(static_cast<size_t>(row)).get());
    }

    QModelIndex BaseTorrentFilesModel::parent(const QModelIndex& child) const {
        if (!child.isValid()) {
            return {};
        }
        TorrentFilesModelDirectory* parentDirectory =
            static_cast<TorrentFilesModelEntry*>(child.internalPointer())->parentDirectory();
        if (parentDirectory == mRootDirectory.get()) {
            return {};
        }
        return createIndex(parentDirectory->row(), 0, parentDirectory);
    }

    int BaseTorrentFilesModel::rowCount(const QModelIndex& parent) const {
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

    void BaseTorrentFilesModel::setFileWanted(const QModelIndex& index, bool wanted) {
        if (index.isValid()) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
            updateDirectoryChildren();
        }
    }

    void BaseTorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted) {
        for (const QModelIndex& index : indexes) {
            if (index.isValid()) {
                static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setWanted(wanted);
            }
        }
        updateDirectoryChildren();
    }

    void BaseTorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntry::Priority priority) {
        if (index.isValid()) {
            static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
            updateDirectoryChildren();
        }
    }

    void
    BaseTorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority) {
        for (const QModelIndex& index : indexes) {
            if (index.isValid()) {
                static_cast<TorrentFilesModelEntry*>(index.internalPointer())->setPriority(priority);
            }
        }
        updateDirectoryChildren();
    }

    void BaseTorrentFilesModel::fileRenamed(TorrentFilesModelEntry* entry, const QString& newName) {
        entry->setName(newName);
        emit dataChanged(createIndex(entry->row(), 0, entry), createIndex(entry->row(), columnCount() - 1, entry));
    }

    void BaseTorrentFilesModel::updateDirectoryChildren(const QModelIndex& parent) {
        const TorrentFilesModelDirectory* directory{};
        if (parent.isValid()) {
            directory = static_cast<const TorrentFilesModelDirectory*>(parent.internalPointer());
        } else if (mRootDirectory) {
            directory = mRootDirectory.get();
        } else {
            return;
        }

        auto changedBatchProcessor = ItemBatchProcessor([&](size_t first, size_t last) {
            emit dataChanged(
                index(static_cast<int>(first), 0, parent),
                index(static_cast<int>(last) - 1, columnCount() - 1, parent)
            );
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
