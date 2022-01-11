/*
 * Tremotesf
 * Copyright (C) 2015-2021 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_ITEMLISTUPDATER_H
#define TREMOTESF_ITEMLISTUPDATER_H

#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace libtremotesf {
    template<typename T, typename U, typename = void>
    inline constexpr bool is_equality_comparable_v = false;

    template<typename T, typename U>
    inline constexpr bool is_equality_comparable_v<T, U, std::enable_if_t<std::is_same_v<decltype(std::declval<T>() == std::declval<U>()), bool>>> = true;

    class ItemBatchProcessor {
    public:
        inline explicit ItemBatchProcessor(std::function<void(size_t, size_t)>&& action) : mAction(std::move(action)) {}

        void nextIndex(size_t index) {
            if (!firstIndex) {
                reset(index);
            } else if (index == *lastIndex) {
                lastIndex = index + 1;
            } else {
                commit();
                reset(index);
            }
        }

        std::optional<size_t> commitIfNeeded() {
            if (firstIndex) {
                return commit();
            }
            return std::nullopt;
        }

        std::optional<size_t> firstIndex = std::nullopt;
        std::optional<size_t> lastIndex = std::nullopt;

    private:
        void reset(size_t index) {
            firstIndex = index;
            lastIndex = index + 1;
        }

        size_t commit() {
            mAction(*firstIndex, *lastIndex);
            const size_t size = *lastIndex - *firstIndex;
            firstIndex = std::nullopt;
            lastIndex = std::nullopt;
            return size;
        }

        std::function<void(size_t, size_t)> mAction;
    };

    template<typename Item, typename NewItem = Item, typename NewItemContainer = std::vector<NewItem>>
    class ItemListUpdater {
    public:
        virtual ~ItemListUpdater() = default;

        virtual void update(std::vector<Item>& items, NewItemContainer&& newItems) {
            if (!items.empty()) {
                auto removedBatchProcessor = ItemBatchProcessor([&](size_t first, size_t last) {
                    onAboutToRemoveItems(first, last);
                    items.erase(items.begin() + static_cast<ptrdiff_t>(first), items.begin() + static_cast<ptrdiff_t>(last));
                    onRemovedItems(first, last);
                });

                auto changedBatchProcessor = ItemBatchProcessor([&](size_t first, size_t last) { onChangedItems(first, last); });

                for (size_t i = 0, max = items.size(); i < max; ++i) {
                    Item* item = &items[i];

                    auto found = findNewItemForItem(newItems, *item);
                    if (found == newItems.end()) {
                        changedBatchProcessor.commitIfNeeded();
                        removedBatchProcessor.nextIndex(i);
                    } else {
                        if (auto size = removedBatchProcessor.commitIfNeeded(); size) {
                            i -= *size;
                            max -= *size;
                            item = &items[i];
                        }
                        if (updateItem(*item, std::forward<NewItem>(*found))) {
                            changedBatchProcessor.nextIndex(i);
                        } else {
                            changedBatchProcessor.commitIfNeeded();
                        }
                        newItems.erase(found);
                    }
                }

                removedBatchProcessor.commitIfNeeded();
                changedBatchProcessor.commitIfNeeded();
            }

            if (!newItems.empty()) {
                const size_t count = newItems.size();
                onAboutToAddItems(count);
                items.reserve(items.size() + count);
                for (auto&& newItem : newItems) {
                    items.push_back(createItemFromNewItem(std::forward<NewItem>(newItem)));
                }
                onAddedItems(count);
            }
        }

    protected:
        /**
         * @brief Find NewItem for corresponing Item with the same identity
         * @param container container of NewItems
         * @param item Item that should be found in container
         * @return iterator to the NewItem with the same identity as Item, or end iterator
         * Default implementation simply checks for equality of items or throws logic_error if they are not comparable
         */
        inline virtual typename NewItemContainer::iterator findNewItemForItem(NewItemContainer& container, const Item& item) {
            if constexpr (is_equality_comparable_v<NewItem, Item>) {
                return std::find_if(container.begin(), container.end(), [&item](const NewItem& newItem) {
                    return newItem == item;
                });
            }
            throw std::logic_error("findNewItemForItem() must be implemented");
        };

        virtual void onAboutToRemoveItems(size_t first, size_t last) = 0;
        virtual void onRemovedItems(size_t first, size_t last) = 0;

        /**
         * @brief Update Item from the NewItem with the same identity
         * @param item Item that will be updated
         * @param newItem NewItem, guaranteed to have the same identity as the Item
         * @return true if Item was changed, otherwise false
         * Default implementation simply returns false
         * (with default implementation of findNewItemForItem() Item and NewItem will be always equal)
         */
        inline virtual bool updateItem([[maybe_unused]] Item& item, [[maybe_unused]] NewItem&& newItem) { return false; };
        virtual void onChangedItems(size_t first, size_t last) = 0;

        /**
         * @brief Create Item from NewItem
         * @return new Item instance
         * Default implementation performs implicit conversion from NewItem to Item,
         * or throws logic_error if types are not convertible
         */
        inline virtual Item createItemFromNewItem(NewItem&& newItem) {
            if constexpr (std::is_convertible_v<NewItem, Item>) {
                return std::move(newItem);
            }
            throw std::logic_error("createItemFromNewItem() must be implemented");
        };
        virtual void onAboutToAddItems(size_t count) = 0;
        virtual void onAddedItems(size_t count) = 0;
    };
}

namespace tremotesf {
    using libtremotesf::ItemBatchProcessor;
    using libtremotesf::ItemListUpdater;
}

#endif // TREMOTESF_ITEMLISTUPDATER_H
