/*
 * Tremotesf
 * Copyright (C) 2015-2021 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_STATUSFILTERSMODEL_H
#define TREMOTESF_STATUSFILTERSMODEL_H

#include <vector>

#include "basetorrentsfilterssettingsmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf
{
    class StatusFiltersModel : public BaseTorrentsFiltersSettingsModel
    {
        Q_OBJECT
    public:
        enum Role
        {
            FilterRole = Qt::UserRole
        };

        inline explicit StatusFiltersModel(QObject* parent = nullptr) : BaseTorrentsFiltersSettingsModel(parent) {};

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex& = {}) const override;
        bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

        QModelIndex indexForStatusFilter(tremotesf::TorrentsProxyModel::StatusFilter filter) const;
        QModelIndex indexForTorrentsProxyModelFilter() const override;

    protected:
        void resetTorrentsProxyModelFilter() const override;

    private:
        void update() override;

        struct Item
        {
            TorrentsProxyModel::StatusFilter filter;
            int torrents;
        };

        std::vector<Item> mItems;

        template<typename, typename, typename, typename> friend class ModelListUpdater;
        friend class StatusFiltersModelUpdater;
    };
}

#endif // TREMOTESF_STATUSFILTERSMODEL_H
