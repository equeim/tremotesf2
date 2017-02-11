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

#ifndef TREMOTESF_SELECTIONMODEL_H
#define TREMOTESF_SELECTIONMODEL_H

#include <QObject>
#include <QModelIndexList>

class QAbstractItemModel;
class QItemSelectionModel;

namespace tremotesf
{
    class SelectionModel : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel)
        Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
        Q_PROPERTY(QModelIndex firstSelectedIndex READ firstSelectedIndex)
        Q_PROPERTY(QModelIndexList selectedIndexes READ selectedIndexes)
        Q_PROPERTY(int selectedIndexesCount READ selectedIndexesCount NOTIFY selectionChanged)
    public:
        SelectionModel();

        QAbstractItemModel* model() const;
        void setModel(QAbstractItemModel* model);

        bool hasSelection() const;

        QModelIndex firstSelectedIndex() const;
        QModelIndexList selectedIndexes() const;
        int selectedIndexesCount() const;

        Q_INVOKABLE bool isSelected(const QModelIndex& index) const;
        Q_INVOKABLE bool isSelected(int row, const QModelIndex& parent = QModelIndex()) const;

        Q_INVOKABLE void select(const QModelIndex& index);
        Q_INVOKABLE void select(int row, const QModelIndex& parent = QModelIndex());

        Q_INVOKABLE void selectAll(const QModelIndex& parent = QModelIndex());
        Q_INVOKABLE void clear();

    private:
        QAbstractItemModel* mModel;
        QItemSelectionModel* mSelectionModel;
    signals:
        void selectionChanged();
    };
}

#endif // TREMOTESF_SELECTIONMODEL_H
