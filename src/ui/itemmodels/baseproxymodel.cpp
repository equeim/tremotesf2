// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "baseproxymodel.h"

namespace tremotesf {
    BaseProxyModel::BaseProxyModel(
        QAbstractItemModel* sourceModel, int sortRole, std::optional<int> fallbackColumn, QObject* parent
    )
        : QSortFilterProxyModel(parent), mFallbackColumn(fallbackColumn) {
        setSourceModel(sourceModel);
        setSortRole(sortRole);
        mCollator.setCaseSensitivity(Qt::CaseInsensitive);
        mCollator.setNumericMode(true);
    }

    QModelIndexList BaseProxyModel::sourceIndexes(const QModelIndexList& proxyIndexes) const {
        QModelIndexList indexes;
        indexes.reserve(proxyIndexes.size());
        for (const QModelIndex& index : proxyIndexes) {
            indexes.append(mapToSource(index));
        }
        return indexes;
    }

    bool BaseProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
        std::partial_ordering ord = compare(source_left, source_right);
        if (ord == std::partial_ordering::equivalent
            && mFallbackColumn.has_value()
            && source_left.column() != *mFallbackColumn) {
            ord =
                compare(source_left.siblingAtColumn(*mFallbackColumn), source_right.siblingAtColumn(*mFallbackColumn));
        }
        return ord == std::partial_ordering::less;
    }

    std::partial_ordering
    BaseProxyModel::compare(const QModelIndex& source_left, const QModelIndex& source_right) const {
        const auto role = sortRole();
        const QVariant leftData = source_left.data(role);
        const QVariant rightData = source_right.data(role);
        if (leftData.typeId() == QMetaType::QString && rightData.typeId() == QMetaType::QString) {
            return mCollator.compare(leftData.toString(), rightData.toString()) <=> 0;
        }
        if (QSortFilterProxyModel::lessThan(source_left, source_right)) {
            return std::partial_ordering::less;
        }
        if (leftData.typeId() == rightData.typeId() && leftData == rightData) {
            return std::partial_ordering::equivalent;
        }
        return std::partial_ordering::unordered;
    }
}
