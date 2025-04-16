// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_BASEPROXYMODEL_H
#define TREMOTESF_BASEPROXYMODEL_H

#ifndef Q_MOC_RUN // compare includes concepts and moc can't handle it :/
#    include <compare>
#endif
#include <optional>

#include <QCollator>
#include <QModelIndexList>
#include <QSortFilterProxyModel>

namespace tremotesf {
    class BaseProxyModel : public QSortFilterProxyModel {
        Q_OBJECT

    public:
        explicit BaseProxyModel(
            QAbstractItemModel* sourceModel = nullptr,
            int sortRole = Qt::DisplayRole,
            std::optional<int> fallbackColumn = std::nullopt,
            QObject* parent = nullptr
        );

        QModelIndexList sourceIndexes(const QModelIndexList& proxyIndexes) const;

    protected:
        bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

    private:
        std::partial_ordering compare(const QModelIndex& source_left, const QModelIndex& source_right) const;

        std::optional<int> mFallbackColumn{};
        QCollator mCollator{};
    };
}

#endif // TREMOTESF_BASEPROXYMODEL_H
