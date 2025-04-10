// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ITEMLISTUPDATER_H
#define TREMOTESF_ITEMLISTUPDATER_H

#include <algorithm>
#include <concepts>
#include <functional>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <vector>

#include <QtClassHelperMacros>

namespace tremotesf {
    class ItemBatchProcessor {
    public:
        inline explicit ItemBatchProcessor(std::function<void(size_t, size_t)>&& action) : mAction(std::move(action)) {}

        void nextIndex(size_t index) {
            if (!firstIndex.has_value()) {
                reset(index);
                return;
            }
            if (!lastIndex.has_value()) throw std::logic_error("lastIndex is empty");
            if (index == *lastIndex) {
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
            if (!firstIndex.has_value()) throw std::logic_error("firstIndex is empty");
            if (!lastIndex.has_value()) throw std::logic_error("lastIndex is empty");
            mAction(*firstIndex, *lastIndex);
            const size_t size = *lastIndex - *firstIndex;
            firstIndex = std::nullopt;
            lastIndex = std::nullopt;
            return size;
        }

        std::function<void(size_t, size_t)> mAction;
    };

    template<typename Item, std::ranges::forward_range NewItemsRange = std::vector<Item>>
    class ItemListUpdater {
        using NewItem = std::ranges::range_value_t<NewItemsRange>;

    public:
        ItemListUpdater() = default;
        virtual ~ItemListUpdater() = default;
        Q_DISABLE_COPY_MOVE(ItemListUpdater)

        void update(std::vector<Item>& items, NewItemsRange newItems) {
            if (!items.empty()) {
                auto removedBatchProcessor = ItemBatchProcessor([&](size_t first, size_t last) {
                    onAboutToRemoveItems(first, last);
                    items.erase(
                        items.begin() + static_cast<ptrdiff_t>(first),
                        items.begin() + static_cast<ptrdiff_t>(last)
                    );
                    onRemovedItems(first, last);
                });

                auto changedBatchProcessor =
                    ItemBatchProcessor([&](size_t first, size_t last) { onChangedItems(first, last); });

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
        inline virtual std::ranges::iterator_t<NewItemsRange>
        findNewItemForItem(NewItemsRange& newItems, const Item& item) {
            if constexpr (std::equality_comparable_with<NewItem, Item>) {
                return std::ranges::find(newItems, item);
            } else {
                throw std::logic_error("findNewItemForItem() must be implemented");
            }
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
        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        inline virtual bool updateItem([[maybe_unused]] Item& item, [[maybe_unused]] NewItem&& newItem) {
            return false;
        };
        virtual void onChangedItems(size_t first, size_t last) = 0;

        /**
         * @brief Create Item from NewItem
         * @return new Item instance
         * Default implementation performs implicit conversion from NewItem to Item,
         * or throws logic_error if types are not convertible
         */
        inline virtual Item createItemFromNewItem(NewItem&& newItem) {
            if constexpr (std::convertible_to<NewItem, Item>) {
                return std::move(newItem);
            } else {
                throw std::logic_error("createItemFromNewItem() must be implemented");
            }
        };
        virtual void onAboutToAddItems(size_t count) = 0;
        virtual void onAddedItems(size_t count) = 0;
    };
}

#endif // TREMOTESF_ITEMLISTUPDATER_H
