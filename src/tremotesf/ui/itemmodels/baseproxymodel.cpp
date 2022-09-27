#include "baseproxymodel.h"

namespace tremotesf
{
    BaseProxyModel::BaseProxyModel(QAbstractItemModel* sourceModel, int sortRole, QObject* parent)
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

    QModelIndex BaseProxyModel::sourceIndex(int proxyRow) const
    {
        return mapToSource(index(proxyRow, 0));
    }

    QModelIndexList BaseProxyModel::sourceIndexes(const QModelIndexList& proxyIndexes) const
    {
        QModelIndexList indexes;
        indexes.reserve(proxyIndexes.size());
        for (const QModelIndex& index : proxyIndexes) {
            indexes.append(mapToSource(index));
        }
        return indexes;
    }

    void BaseProxyModel::sort(int column, Qt::SortOrder order)
    {
        QSortFilterProxyModel::sort(column, order);
        emit sortOrderChanged();
    }

    bool BaseProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        QVariant leftVariant(left.data(sortRole()));
        QVariant rightVariant(right.data(sortRole()));
        if (leftVariant.userType() == QMetaType::QString) {
            return (mCollator.compare(leftVariant.toString(), rightVariant.toString()) < 0);
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }
}
