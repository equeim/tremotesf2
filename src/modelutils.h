 /*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_MODELUTILS_H
#define TREMOTESF_MODELUTILS_H

#include <QAbstractItemModel>

#include "libtremotesf/itemlistupdater.h"

namespace tremotesf
{
    template<typename Model, typename Item, typename NewItem = Item, typename NewItemContainer = std::vector<NewItem>>
    class ModelListUpdater : public ItemListUpdater<Item, NewItem, NewItemContainer> {
        static_assert(std::is_base_of_v<QAbstractItemModel, Model>);

    public:
        inline explicit ModelListUpdater(Model& model) : mModel(model) {}

    protected:
        void onChangedItems(size_t first, size_t last) override {
            emit mModel.dataChanged(mModel.index(static_cast<int>(first), lastColumn), mModel.index(static_cast<int>(last - 1), lastColumn));
        }

        void onAboutToRemoveItems(size_t first, size_t last) override {
            mModel.beginRemoveRows({}, static_cast<int>(first), static_cast<int>(last - 1));
        };

        void onRemovedItems(size_t, size_t) override {
            mModel.endRemoveRows();
        }

        void onAboutToAddItems(size_t count) override {
            const int first = mModel.rowCount();
            mModel.beginInsertRows({}, first, first + static_cast<int>(count) - 1);
        }

        void onAddedItems(size_t) override {
            mModel.endInsertRows();
        };

    protected:
        Model& mModel;
        const int lastColumn = static_cast<QAbstractItemModel&>(mModel).columnCount() - 1;
    };
}

#endif // TREMOTESF_MODELUTILS_H
