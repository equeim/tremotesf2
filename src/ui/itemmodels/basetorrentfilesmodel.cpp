// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "basetorrentfilesmodel.h"

#include <ranges>

#include <QCoreApplication>
#include <QHash>
#include <QIcon>
#include <QSet>

#include "desktoputils.h"
#include "formatutils.h"
#include "stdutils.h"

namespace tremotesf {
    BaseTorrentFilesModel::BaseTorrentFilesModel(std::vector<Column>&& columns, QObject* parent)
        : QAbstractItemModel(parent), mColumns(std::move(columns)) {}

    int BaseTorrentFilesModel::columnCount(const QModelIndex&) const { return static_cast<int>(mColumns.size()); }

    QVariant BaseTorrentFilesModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        const auto* const entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
        const Column column = fromColumnNumber(index.column());
        switch (role) {
        case Qt::CheckStateRole:
            if (column == Column::Name) {
                using enum TorrentFilesModelEntry::WantedState;
                switch (entry->wantedState()) {
                case Wanted:
                    return Qt::Checked;
                case Unwanted:
                    return Qt::Unchecked;
                case Mixed:
                    return Qt::PartiallyChecked;
                }
            }
            break;
        case Qt::DecorationRole:
            if (column == Column::Name) {
                return entry->icon();
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
        if (fromColumnNumber(index.column()) == Column::Name) {
            return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
        }
        return QAbstractItemModel::flags(index);
    }

    QVariant BaseTorrentFilesModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (fromColumnNumber(section)) {
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
        if (fromColumnNumber(index.column()) == Column::Name && role == Qt::CheckStateRole) {
            setFilesWanted({index}, (value.toInt() == Qt::Checked));
            return true;
        }
        return false;
    }

    QModelIndex BaseTorrentFilesModel::index(int row, int column, const QModelIndex& parent) const {
        if (!parent.isValid()) {
            if (row == 0 && mRootEntry) {
                return createIndex(0, column, mRootEntry.get());
            }
            return {};
        }
        const auto* const parentEntry = static_cast<TorrentFilesModelEntry*>(parent.internalPointer());
        if (!parentEntry->isDirectory()) {
            return {};
        }
        return createIndex(row, column, &parentEntry->children().at(static_cast<size_t>(row)));
    }

    QModelIndex BaseTorrentFilesModel::parent(const QModelIndex& child) const {
        if (!child.isValid()) {
            return {};
        }
        const auto* const parentDirectoryEntry =
            static_cast<TorrentFilesModelEntry*>(child.internalPointer())->parentDirectory();
        if (!parentDirectoryEntry) {
            return {};
        }
        return createIndex(parentDirectoryEntry->row(), 0, parentDirectoryEntry);
    }

    int BaseTorrentFilesModel::rowCount(const QModelIndex& parent) const {
        if (parent.isValid()) {
            const auto* const entry = static_cast<TorrentFilesModelEntry*>(parent.internalPointer());
            if (entry->isDirectory()) {
                return static_cast<int>(entry->children().size());
            }
        } else if (mRootEntry) {
            return 1;
        }
        return 0;
    }

    namespace {
        class DataChangedDispatcher final {
        public:
            struct ColumnAndRoles {
                int column;
                QList<int> roles;
            };

            explicit DataChangedDispatcher(std::optional<ColumnAndRoles> columnAndRoles = std::nullopt)
                : mColumnAndRoles(std::move(columnAndRoles)) {}

            void add(const QModelIndex& parent, int row) {
                // NOLINTNEXTLINE(clazy-detaching-member)
                if (const auto found = mPendingSignals.find(parent); found != mPendingSignals.end()) {
                    found->push_back(row);
                } else {
                    mPendingSignals.emplace(parent, std::vector{row});
                }
            }
            void dispatchSignals(QAbstractItemModel& model) {
                for (auto&& [parent, rows] : mPendingSignals.asKeyValueRange()) {
                    if (rows.size() == 1) {
                        const int row = rows.front();
                        if (mColumnAndRoles.has_value()) {
                            const auto index = model.index(row, static_cast<int>(mColumnAndRoles->column), parent);
                            emit model.dataChanged(index, index, mColumnAndRoles->roles);
                        } else {
                            emit model.dataChanged(
                                model.index(row, 0, parent),
                                model.index(row, model.columnCount() - 1, parent)
                            );
                        }
                        continue;
                    }
                    std::ranges::sort(rows);
                    int firstRow = rows.front();
                    int lastRow = firstRow;
                    const auto emitForRange = [&] {
                        if (mColumnAndRoles.has_value()) {
                            emit model.dataChanged(
                                model.index(firstRow, mColumnAndRoles->column, parent),
                                model.index(lastRow, mColumnAndRoles->column, parent),
                                mColumnAndRoles->roles
                            );
                        } else {
                            emit model.dataChanged(
                                model.index(firstRow, 0, parent),
                                model.index(lastRow, model.columnCount() - 1, parent)
                            );
                        }
                    };
                    for (int row : rows | std::views::drop(1)) {
                        if (row != (lastRow + 1)) {
                            emitForRange();
                            firstRow = row;
                        }
                        lastRow = row;
                    }
                    emitForRange();
                }
            }

        private:
            std::optional<ColumnAndRoles> mColumnAndRoles;
            QHash<QModelIndex, std::vector<int>> mPendingSignals{};
        };

        void recalculateDirectoryAndItsParents(
            TorrentFilesModelEntry* directory, QModelIndex index, DataChangedDispatcher& dispatcher
        ) {
            while (index.isValid()) {
                if (!directory->recalculateFromChildren()) {
                    break;
                }
                dispatcher.add(index.parent(), index.row());
                directory = directory->parentDirectory();
                index = index.parent();
            }
        }

        template<std::invocable<TorrentFilesModelEntry*> UpdateState>
            requires std::same_as<std::invoke_result_t<UpdateState, TorrentFilesModelEntry*>, bool>
        void updateDirectoryChildrenRecursively(
            TorrentFilesModelEntry* directory,
            const QModelIndex& directoryIndex,
            UpdateState updateState,
            BaseTorrentFilesModel& model,
            DataChangedDispatcher& dispatcher
        ) {
            for (auto& entry : directory->children()) {
                if (updateState(&entry)) {
                    dispatcher.add(directoryIndex, entry.row());
                }
                if (entry.isDirectory()) {
                    updateDirectoryChildrenRecursively(
                        &entry,
                        model.index(entry.row(), 0, directoryIndex),
                        updateState,
                        model,
                        dispatcher
                    );
                }
            }
        }

        template<std::invocable<TorrentFilesModelEntry*> UpdateState>
            requires std::same_as<std::invoke_result_t<UpdateState, TorrentFilesModelEntry*>, bool>
        void setWantedOrPriority(
            const QModelIndexList& indexes,
            DataChangedDispatcher::ColumnAndRoles columnAndRoles,
            UpdateState updateState,
            BaseTorrentFilesModel& model
        ) {
            if (indexes.empty()) return;
            if (!std::ranges::all_of(indexes, &QModelIndex::isValid)) return;

            DataChangedDispatcher dispatcher(std::move(columnAndRoles));
            QSet<std::pair<TorrentFilesModelEntry*, QModelIndex>> parentDirectoriesToRecalculate{};

            for (const auto& index : indexes) {
                auto* const entry = static_cast<TorrentFilesModelEntry*>(index.internalPointer());
                if (updateState(entry)) {
                    dispatcher.add(index.parent(), index.row());
                    if (entry->isDirectory()) {
                        updateDirectoryChildrenRecursively(entry, index, updateState, model, dispatcher);
                    }
                    parentDirectoriesToRecalculate.insert({entry->parentDirectory(), index.parent()});
                }
            }

            for (const auto& [directory, index] : parentDirectoriesToRecalculate) {
                recalculateDirectoryAndItsParents(directory, index, dispatcher);
            }
            dispatcher.dispatchSignals(model);
        }
    }
    void BaseTorrentFilesModel::setFilesWanted(const QModelIndexList& indexes, bool wanted) {
        setWantedOrPriority(
            indexes,
            {.column = columnNumber(Column::Name), .roles = {Qt::CheckStateRole}},
            [&](TorrentFilesModelEntry* entry) { return entry->setWanted(wanted); },
            *this
        );
    }

    void
    BaseTorrentFilesModel::setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority) {
        setWantedOrPriority(
            indexes,
            {.column = columnNumber(Column::Priority), .roles = {Qt::DisplayRole, SortRole}},
            [&](TorrentFilesModelEntry* entry) { return entry->setPriority(priority); },
            *this
        );
    }

    void BaseTorrentFilesModel::fileRenamed(TorrentFilesModelEntry* entry, const QString& newName) {
        entry->setName(newName);
        emit dataChanged(createIndex(entry->row(), 0, entry), createIndex(entry->row(), columnCount() - 1, entry));
    }

    int BaseTorrentFilesModel::columnNumber(Column column) const {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        return indexOfCasted<int>(mColumns, column).value();
    }

    BaseTorrentFilesModel::Column BaseTorrentFilesModel::fromColumnNumber(int column) const {
        return mColumns.at(static_cast<size_t>(column));
    }

    void BaseTorrentFilesModel::updateFiles(
        std::span<const int> changedFiles, std::function<void(size_t, TorrentFilesModelEntry*)>&& updateFile
    ) {
        if (changedFiles.empty()) return;

        DataChangedDispatcher dispatcher{};

        for (int index : changedFiles) {
            const auto sIndex = static_cast<size_t>(index);
            auto* const file = mFiles.at(sIndex);
            updateFile(sIndex, file);
            auto* const parent = file->parentDirectory();
            const auto parentIndex = [&] {
                if (!parent) {
                    return QModelIndex{};
                }
                return createIndex(parent->row(), 0, parent);
            }();
            dispatcher.add(parentIndex, file->row());
            recalculateDirectoryAndItsParents(parent, parentIndex, dispatcher);
        }

        dispatcher.dispatchSignals(*this);
    }
}
