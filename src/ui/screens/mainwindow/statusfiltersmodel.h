// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_STATUSFILTERSMODEL_H
#define TREMOTESF_STATUSFILTERSMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf {
    class StatusFiltersModel final : public BaseTorrentsFiltersSettingsModel {
        Q_OBJECT

    public:
        static constexpr auto FilterRole = Qt::UserRole;

        inline explicit StatusFiltersModel(QObject* parent = nullptr) : BaseTorrentsFiltersSettingsModel(parent){};

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        QModelIndex indexForStatusFilter(TorrentsProxyModel::StatusFilter filter) const;
        QModelIndex indexForTorrentsProxyModelFilter() const override;

        using QAbstractItemModel::beginInsertRows;
        using QAbstractItemModel::beginRemoveRows;
        using QAbstractItemModel::endInsertRows;
        using QAbstractItemModel::endRemoveRows;

        struct Item {
            TorrentsProxyModel::StatusFilter filter;
            int torrents;
        };

    protected:
        void resetTorrentsProxyModelFilter() const override;

    private:
        void update() override;

        std::vector<Item> mItems;
    };
}

#endif // TREMOTESF_STATUSFILTERSMODEL_H
