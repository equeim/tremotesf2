// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ALLTRACKERSMODEL_H
#define TREMOTESF_ALLTRACKERSMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"

namespace tremotesf {
    class AllTrackersModel final : public BaseTorrentsFiltersSettingsModel {
        Q_OBJECT

    public:
        static constexpr auto TrackerRole = Qt::UserRole;

        inline explicit AllTrackersModel(QObject* parent = nullptr) : BaseTorrentsFiltersSettingsModel(parent) {};

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;

        QModelIndex indexForTracker(const QString& tracker) const;

        QModelIndex indexForTorrentsProxyModelFilter() const override;

        using QAbstractItemModel::beginInsertRows;
        using QAbstractItemModel::beginRemoveRows;
        using QAbstractItemModel::endInsertRows;
        using QAbstractItemModel::endRemoveRows;

        struct TrackerItem {
            QString tracker;
            int torrents;
        };

    protected:
        void resetTorrentsProxyModelFilter() const override;

    private:
        void update() override;

        std::vector<TrackerItem> mTrackers;
    };
}

#endif // TREMOTESF_ALLTRACKERSMODEL_H
