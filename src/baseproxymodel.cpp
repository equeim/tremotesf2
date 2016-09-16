/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include "baseproxymodel.h"

namespace tremotesf
{
    BaseProxyModel::BaseProxyModel(QAbstractItemModel* sourceModel, int sortRole, QObject *parent)
        : QSortFilterProxyModel(parent)
    {
        setSourceModel(sourceModel);
        QSortFilterProxyModel::setSortRole(sortRole);
        mCollator.setCaseSensitivity(Qt::CaseInsensitive);
        mCollator.setNumericMode(true);
    }

    QModelIndex BaseProxyModel::sourceIndex(const QModelIndex& proxyIndex) const
    {
        return mapToSource(proxyIndex);
    }

    QModelIndexList BaseProxyModel::sourceIndexes(const QModelIndexList& proxyIndexes) const
    {
        QModelIndexList indexes;
        for (const QModelIndex& index : proxyIndexes) {
            indexes.append(mapToSource(index));
        }
        return indexes;
    }

    int BaseProxyModel::sourceRow(int proxyRow, const QModelIndex& proxyParent) const
    {
        return mapToSource(index(proxyRow, 0, proxyParent)).row();
    }

    void BaseProxyModel::setSortRole(int role)
    {
        QSortFilterProxyModel::setSortRole(role);
        emit sortRoleChanged();
    }

    void BaseProxyModel::sort(int column, Qt::SortOrder order)
    {
        QSortFilterProxyModel::sort(column, order);
    }

    bool BaseProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        QVariant leftVariant(left.data(sortRole()));
        QVariant rightVariant(right.data(sortRole()));
        if (leftVariant.userType() == QVariant::String) {
            return (mCollator.compare(leftVariant.toString(), rightVariant.toString()) < 0);
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }
}
