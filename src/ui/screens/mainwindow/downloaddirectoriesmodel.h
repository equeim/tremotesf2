// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_DOWNLOADDIRECTORIESMODEL_H
#define TREMOTESF_DOWNLOADDIRECTORIESMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"

namespace tremotesf {
    class DownloadDirectoriesModel final : public BaseTorrentsFiltersSettingsModel {
        Q_OBJECT

    public:
        enum class Role { Directory = Qt::UserRole };

        explicit DownloadDirectoriesModel(QObject* parent = nullptr);

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;

        QModelIndex indexForTorrentsProxyModelFilter() const override;

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
        bool mDisplayFullDownloadDirectoryPath{};
    };
}

#endif // TREMOTESF_DOWNLOADDIRECTORIESMODEL_H
