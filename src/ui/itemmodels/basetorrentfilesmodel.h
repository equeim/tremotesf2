// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_BASETORRENTFILESMODEL_H
#define TREMOTESF_BASETORRENTFILESMODEL_H

#include <functional>
#include <memory>
#include <vector>
#include <QAbstractItemModel>

#include "torrentfilesmodelentry.h"

namespace tremotesf {
    class BaseTorrentFilesModel : public QAbstractItemModel {
        Q_OBJECT

    public:
        enum class Column { Name, Size, ProgressBar, Progress, Priority };
        static constexpr auto SortRole = Qt::UserRole;

        explicit BaseTorrentFilesModel(std::vector<Column>&& columns, QObject* parent = nullptr);

        int columnCount(const QModelIndex& = {}) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent = {}) const override;

        virtual void setFilesWanted(const QModelIndexList& indexes, bool wanted);
        virtual void setFilesPriority(const QModelIndexList& indexes, TorrentFilesModelEntry::Priority priority);

        virtual void renameFile(const QModelIndex& index, const QString& newName) = 0;
        void fileRenamed(TorrentFilesModelEntry* entry, const QString& newName);

    protected:
        void updateFiles(
            std::span<const int> changedFiles, std::function<void(size_t, TorrentFilesModelFile*)>&& updateFile
        );

        std::unique_ptr<TorrentFilesModelDirectory> mRootDirectory{};
        std::vector<TorrentFilesModelFile*> mFiles{};

    private:
        const std::vector<Column> mColumns;
    };
}

#endif // TREMOTESF_BASETORRENTFILESMODEL_H
