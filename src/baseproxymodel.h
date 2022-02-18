/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_BASEPROXYMODEL_H
#define TREMOTESF_BASEPROXYMODEL_H

#include <QCollator>
#include <QModelIndexList>
#include <QSortFilterProxyModel>

namespace tremotesf
{
    class BaseProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT
        Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder NOTIFY sortOrderChanged)
    public:
        explicit BaseProxyModel(QAbstractItemModel* sourceModel = nullptr, int sortRole = Qt::DisplayRole, QObject* parent = nullptr);

        Q_INVOKABLE QModelIndex sourceIndex(const QModelIndex& proxyIndex) const;
        Q_INVOKABLE QModelIndex sourceIndex(int proxyRow) const;
        Q_INVOKABLE QModelIndexList sourceIndexes(const QModelIndexList& proxyIndexes) const;

        Q_INVOKABLE void sort(int column = 0, Qt::SortOrder order = Qt::AscendingOrder) override;

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    private:
        QCollator mCollator;

    signals:
        void sortOrderChanged();
    };
}

#endif // TREMOTESF_BASEPROXYMODEL_H
