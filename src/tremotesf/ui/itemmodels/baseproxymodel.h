// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_BASEPROXYMODEL_H
#define TREMOTESF_BASEPROXYMODEL_H

#include <QCollator>
#include <QModelIndexList>
#include <QSortFilterProxyModel>

namespace tremotesf {
    class BaseProxyModel : public QSortFilterProxyModel {
        Q_OBJECT
    public:
        explicit BaseProxyModel(
            QAbstractItemModel* sourceModel = nullptr, int sortRole = Qt::DisplayRole, QObject* parent = nullptr
        );

        QModelIndex sourceIndex(const QModelIndex& proxyIndex) const;
        QModelIndex sourceIndex(int proxyRow) const;
        QModelIndexList sourceIndexes(const QModelIndexList& proxyIndexes) const;

        void sort(int column = 0, Qt::SortOrder order = Qt::AscendingOrder) override;

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
        QCollator mCollator;

    signals:
        void sortOrderChanged();
    };
}

#endif // TREMOTESF_BASEPROXYMODEL_H
