/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "selectionmodel.h"

#include <QAbstractItemModel>
#include <QItemSelectionModel>

namespace tremotesf
{
    SelectionModel::SelectionModel()
        : mModel(nullptr)
    {
    }

    QAbstractItemModel* SelectionModel::model() const
    {
        return mModel;
    }

    void SelectionModel::setModel(QAbstractItemModel* model)
    {
        if (model && !mModel) {
            mModel = model;
            mSelectionModel = new QItemSelectionModel(mModel, this);
            QObject::connect(mSelectionModel, &QItemSelectionModel::selectionChanged, this, &SelectionModel::selectionChanged);
        }
    }

    bool SelectionModel::hasSelection() const
    {
        if (mModel) {
            return mSelectionModel->hasSelection();
        }
        return false;
    }

    QModelIndex SelectionModel::firstSelectedIndex() const
    {
        if (mModel && mSelectionModel->hasSelection()) {
            return mSelectionModel->selectedIndexes().first();
        }
        return QModelIndex();
    }

    QModelIndexList SelectionModel::selectedIndexes() const
    {
        if (mModel) {
            return mSelectionModel->selectedIndexes();
        }
        return QModelIndexList();
    }

    int SelectionModel::selectedIndexesCount() const
    {
        if (mModel) {
            return mSelectionModel->selectedIndexes().size();
        }
        return 0;
    }

    bool SelectionModel::isSelected(const QModelIndex& index) const
    {
        if (mModel) {
            return mSelectionModel->isSelected(index);
        }
        return false;
    }

    bool SelectionModel::isSelected(int row, const QModelIndex& parent) const
    {
        if (mModel) {
            return mSelectionModel->isSelected(mModel->index(row, 0, parent));
        }
        return false;
    }

    void SelectionModel::select(const QModelIndex& index)
    {
        if (mModel) {
            mSelectionModel->select(index, QItemSelectionModel::Toggle);
        }
    }

    void SelectionModel::select(int row, const QModelIndex& parent)
    {
        if (mModel) {
            mSelectionModel->select(mModel->index(row, 0, parent), QItemSelectionModel::Toggle);
        }
    }

    void SelectionModel::selectAll(const QModelIndex& parent)
    {
        if (mModel) {
            mSelectionModel->select(QItemSelection(mModel->index(0, 0, parent),
                                                   mModel->index(mModel->rowCount(parent) - 1,
                                                                 mModel->columnCount(parent) - 1,
                                                                 parent)),
                                    QItemSelectionModel::Select);
        }
    }

    void SelectionModel::clear()
    {
        if (mModel) {
            mSelectionModel->clear();
        }
    }
}
