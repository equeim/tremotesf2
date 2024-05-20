// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "basetorrentfilesmodel.h"

#include <algorithm>

#include <QApplication>
#include <QStyle>

#include "itemlistupdater.h"
#include "formatutils.h"
#include "log/log.h"

#include <fmt/ranges.h>

SPECIALIZE_FORMATTER_FOR_QDEBUG(QModelIndex)

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
                case TorrentFilesModelEntry::WantedState::Wanted:
                    return Qt::Checked;
                case TorrentFilesModelEntry::WantedState::Unwanted:
                    return Qt::Unchecked;
                case TorrentFilesModelEntry::WantedState::Mixed:
                    return Qt::PartiallyChecked;
                }
            }
            break;
        case Qt::DecorationRole:
            if (column == Column::Name) {
                if (entry->isDirectory()) {
                    return qApp->style()->standardIcon(QStyle::SP_DirIcon);
                }
                return qApp->style()->standardIcon(QStyle::SP_FileIcon);
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
                return QVariant::fromValue(entry->priority());
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
        TorrentFilesModelDirectory* parentDirectory{};
        if (parent.isValid()) {
            parentDirectory = static_cast<TorrentFilesModelDirectory*>(parent.internalPointer());
        } else if (mRootDirectory) {
            parentDirectory = mRootDirectory.get();
        } else {
            return {};
        }
        return createIndex(row, column, parentDirectory->childAtRow(row));
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
                return static_cast<const TorrentFilesModelDirectory*>(entry)->childrenCount();
            }
            return 0;
        }
        if (mRootDirectory) {
            return mRootDirectory->childrenCount();
        }
        return 0;
    }

    void BaseTorrentFilesModel::setFileWanted(const QModelIndex& index, bool wanted) {
        if (index.isValid()) {
            setFilesWanted({index}, wanted);
        }
    }

    void BaseTorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted) {
        info().log("setFilesWanted: {}, {}", indexes, wanted);
        if (indexes.empty()) {
            return;
        }
        std::vector<TorrentFilesModelDirectory*> directoriesToRecalculate{};
        for (const QModelIndex& index : indexes) {
            if (index.isValid()) {
                auto* entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                entry->setWanted(wanted);
                addParentDirectoriesToRecalculateList(entry, directoriesToRecalculate);
            }
        }
        info().log(
            "directoriesToRecalculate = {}",
            directoriesToRecalculate | std::views::transform(&TorrentFilesModelEntry::name)
        );
        std::ranges::for_each(directoriesToRecalculate, &TorrentFilesModelDirectory::recalculateFromChildren);
        emitDataChangedForChildren();
    }

    void BaseTorrentFilesModel::setFilePriority(const QModelIndex& index, TorrentFilesModelEntry::Priority priority) {
        if (index.isValid()) {
            setFilesPriority({index}, priority);
        }
    }

    void
    BaseTorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority) {
        if (indexes.empty()) {
            return;
        }
        std::vector<TorrentFilesModelDirectory*> directoriesToRecalculate{};
        for (const QModelIndex& index : indexes) {
            if (index.isValid()) {
                auto* const entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                entry->setPriority(priority);
                addParentDirectoriesToRecalculateList(entry, directoriesToRecalculate);
            }
        }
        std::ranges::for_each(directoriesToRecalculate, &TorrentFilesModelDirectory::recalculateFromChildren);
        emitDataChangedForChildren();
    }

    void BaseTorrentFilesModel::fileRenamed(TorrentFilesModelEntry* entry, const QString& newName) {
        entry->setName(newName);
        emit dataChanged(createIndex(entry->row(), 0, entry), createIndex(entry->row(), columnCount() - 1, entry));
    }

    void BaseTorrentFilesModel::addParentDirectoriesToRecalculateList(
        TorrentFilesModelEntry* entry, std::vector<TorrentFilesModelDirectory*>& directoriesToRecalculate
    ) {
        auto* directory = entry->parentDirectory();
        ptrdiff_t indexToInsert = 0;
        while (directory != mRootDirectory.get()) {
            const auto found = std::ranges::find(directoriesToRecalculate, directory);
            if (found != directoriesToRecalculate.end()) {
                break;
            }
            directoriesToRecalculate.insert(directoriesToRecalculate.begin() + indexToInsert, directory);
            ++indexToInsert;
            directory = directory->parentDirectory();
        }
    }

    void BaseTorrentFilesModel::emitDataChangedForChildren(const QModelIndex& parent) {
        info().log("emitDataChangedForChildren: parent = {}", parent);
        TorrentFilesModelDirectory* directory{};
        if (parent.isValid()) {
            directory = static_cast<TorrentFilesModelDirectory*>(parent.internalPointer());
        } else if (mRootDirectory) {
            directory = mRootDirectory.get();
        } else {
            info().log("emitDataChangedForChildren: nope");
            return;
        }

        auto changedBatchProcessor = ItemBatchProcessor([&](size_t first, size_t last) {
            emit dataChanged(
                index(static_cast<int>(first), 0, parent),
                index(static_cast<int>(last) - 1, columnCount() - 1, parent)
            );
        });
        directory->forEachChild([&](TorrentFilesModelEntry* child) {
            if (child->consumeChangedMark()) {
                info().log("changed");
                changedBatchProcessor.nextIndex(static_cast<size_t>(child->row()));
            } else {
                info().log("not changed");
            }
            if (child->isDirectory()) {
                info().log("descend");
                emitDataChangedForChildren(index(child->row(), 0, parent));
            }
        });

        changedBatchProcessor.commitIfNeeded();
    }
}
