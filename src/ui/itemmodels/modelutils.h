// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_MODELUTILS_H
#define TREMOTESF_MODELUTILS_H

#include <concepts>
#include <QAbstractItemModel>

#include "itemlistupdater.h"

namespace tremotesf {
    template<
        std::derived_from<QAbstractItemModel> Model,
        typename Item,
        std::ranges::forward_range NewItemsRange = std::vector<Item>>
    class ModelListUpdater : public ItemListUpdater<Item, NewItemsRange> {


    public:
        inline explicit ModelListUpdater(Model& model) : mModel(model) {}

    protected:
        void onChangedItems(size_t first, size_t last) override {
            emit mModel.dataChanged(
                mModel.index(static_cast<int>(first), 0),
                mModel.index(static_cast<int>(last - 1), lastColumn)
            );
        }

        void onAboutToRemoveItems(size_t first, size_t last) override {
            mModel.beginRemoveRows({}, static_cast<int>(first), static_cast<int>(last - 1));
        };

        void onRemovedItems(size_t, size_t) override { mModel.endRemoveRows(); }

        void onAboutToAddItems(size_t count) override {
            const int first = mModel.rowCount();
            mModel.beginInsertRows({}, first, first + static_cast<int>(count) - 1);
        }

        void onAddedItems(size_t) override { mModel.endInsertRows(); };

        Model& mModel;
        const int lastColumn = static_cast<QAbstractItemModel&>(mModel).columnCount() - 1;
    };
}

#endif // TREMOTESF_MODELUTILS_H
