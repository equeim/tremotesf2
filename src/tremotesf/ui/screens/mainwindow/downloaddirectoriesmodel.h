// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_DOWNLOADDIRECTORIESMODEL_H
#define TREMOTESF_DOWNLOADDIRECTORIESMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"

namespace tremotesf {
    class DownloadDirectoriesModel : public BaseTorrentsFiltersSettingsModel {
        Q_OBJECT
    public:
        static constexpr auto DirectoryRole = Qt::UserRole;

        inline explicit DownloadDirectoriesModel(QObject* parent = nullptr)
            : BaseTorrentsFiltersSettingsModel(parent){};

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        QModelIndex indexForDirectory(const QString& downloadDirectory) const;

        QModelIndex indexForTorrentsProxyModelFilter() const override;

        using QAbstractItemModel::beginInsertRows;
        using QAbstractItemModel::beginRemoveRows;
        using QAbstractItemModel::endInsertRows;
        using QAbstractItemModel::endRemoveRows;

        struct DirectoryItem {
            QString directory{};
            QString displayDirectory{};
            int torrents{};
        };

    protected:
        void resetTorrentsProxyModelFilter() const override;

    private:
        void update() override;

        std::vector<DirectoryItem> mDirectories{};
    };
}

#endif // TREMOTESF_DOWNLOADDIRECTORIESMODEL_H
