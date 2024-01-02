// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <exception>
#include <optional>
#include <set>

#include <QTest>

#include <fmt/ranges.h>

#include "log/log.h"
#include "itemlistupdater.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-do-while)

struct Item {
    int id;
    QString data;

    bool operator==(const Item& other) const = default;
    bool operator<(const Item& other) const { return id < other.id; }
};

namespace fmt {
    template<>
    struct formatter<Item> : tremotesf::SimpleFormatter {
        format_context::iterator format(const Item& item, format_context& ctx) FORMAT_CONST {
            return fmt::format_to(ctx.out(), "Item(id={}, data={})", item.id, item.data);
        }
    };
}

class AbortTest : public std::exception {};

#define QVERIFY_THROW(statement)                                                                                  \
    do {                                                                                                          \
        if (!QTest::qVerify(static_cast<bool>(statement), #statement, "", __FILE__, __LINE__)) throw AbortTest(); \
    } while (false)

#define QCOMPARE_THROW(actual, expected)                                                                   \
    do {                                                                                                   \
        if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) throw AbortTest(); \
    } while (false)

class Updater final : public tremotesf::ItemListUpdater<Item> {
public:
    std::vector<std::pair<size_t, size_t>> aboutToRemoveIndexRanges;
    std::vector<std::pair<size_t, size_t>> removedIndexRanges;
    std::vector<std::pair<size_t, size_t>> changedIndexRanges;
    std::optional<size_t> aboutToAddCount;
    std::optional<size_t> addedCount;

protected:
    typename std::vector<Item>::iterator findNewItemForItem(std::vector<Item>& container, const Item& item) override {
        return std::find_if(container.begin(), container.end(), [&item](const Item& newItem) {
            return newItem.id == item.id;
        });
    };

    void onAboutToRemoveItems(size_t first, size_t last) override {
        aboutToRemoveIndexRanges.emplace_back(first, last);
    }

    void onRemovedItems(size_t first, size_t last) override { removedIndexRanges.emplace_back(first, last); }

    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    bool updateItem(Item& item, Item&& newItem) override {
        if (item != newItem) {
            item = newItem;
            return true;
        }
        return false;
    }

    void onChangedItems(size_t first, size_t last) override { changedIndexRanges.emplace_back(first, last); }

    Item createItemFromNewItem(Item&& newItem) override { return std::move(newItem); }

    void onAboutToAddItems(size_t count) override {
        if (aboutToAddCount) {
            QFAIL("onAboutToAddItems() must be called only once");
        }
        aboutToAddCount = count;
    }

    void onAddedItems(size_t count) override {
        if (addedCount) {
            QFAIL("onAddedItems() must be called only once");
        }
        addedCount = count;
    }
};

class ItemListUpdaterTest final : public QObject {
    Q_OBJECT

private slots:
    void emptyListDidNotChange() { checkUpdate({}, {}); }

    void singleItemListDidNotChange() { checkUpdate({{42, "666"}}, {{42, "666"}}); }

    void multipleItemsListDidNotChange() {
        checkUpdate({{42, "666"}, {1, "Foo"}, {11, "Bar"}}, {{42, "666"}, {1, "Foo"}, {11, "Bar"}});
    }

    void addedSingleItemToEmptyList() { checkUpdate({}, {{42, "666"}}); }

    void addedMultipleItemsToEmptyList() { checkUpdate({}, {{42, "666"}, {1, "Foo"}}); }

    void addedSingleItemToNonEmptyList() { checkUpdate({{42, "666"}}, {Item{42, "666"}, {1, "Foo"}}); }

    void addedMultipleItemsToNonEmptyList() { checkUpdate({{42, "666"}}, {{42, "666"}, {1, "Foo"}, {11, "Bar"}}); }

    void removedItemFromSingleItemList() { checkUpdate({{42, "666"}}, {}); }

    void removedLastItemFromMultipleItemsList() { checkUpdate({{42, "666"}, {666, ""}}, {{42, "666"}}); }

    void removedFirstItemFromMultipleItemsList() { checkUpdate({{42, "666"}, {666, ""}}, {{666, ""}}); }

    void removedMiddleItemFromMultipleItemsList() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}}, {{42, "666"}, {1, ""}});
    }

    void removedTwoConsecutiveItemsFromEnd() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}}, {{42, "666"}, {666, ""}});
    }

    void removedTwoConsecutiveItemsFromBeginning() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}}, {{1, ""}, {18, "Nope"}});
    }

    void removedTwoConsecutiveItemsFromMiddle() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}}, {{42, "666"}, {18, "Nope"}});
    }

    void removedTwoSeparateItems1() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "666"}, {666, ""}, {18, "Nope"}}
        );
    }

    void removedTwoSeparateItems2() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{666, ""}, {18, "Nope"}, {77, "Foo"}}
        );
    }

    void removedTwoSeparateItems3() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}}, {{666, ""}, {1, ""}, {18, "Nope"}});
    }

    void changedSingleItemList() { checkUpdate({{42, "666"}}, {{42, "42"}}); }

    void changedFirstItemInMultipleItemList() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}}, {{42, "42"}, {666, ""}, {1, ""}});
    }

    void changedMiddleItemInMultipleItemList() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}}, {{42, "666"}, {666, "666"}, {1, ""}});
    }

    void changedLastItemInMultipleItemList() {
        checkUpdate({{42, "666"}, {666, ""}, {1, ""}}, {{42, "666"}, {666, ""}, {1, "nnn"}});
    }

    void changedTwoConsecutiveItemsFromBeginning() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "AAAA"}, {666, "ok"}, {1, ""}, {18, "Nope"}, {77, "Foo"}}
        );
    }

    void changedTwoConsecutiveItemsFromMiddle() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "666"}, {666, ""}, {1, "uhh"}, {18, ""}, {77, "Foo"}}
        );
    }

    void changedTwoConsecutiveItemsFromEnd() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "666"}, {666, ""}, {1, ""}, {18, "arr"}, {77, "qr"}}
        );
    }

    void changedTwoSeparateItems1() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "667"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Fooo"}}
        );
    }

    void changedTwoSeparateItems2() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "666"}, {666, "sure"}, {1, ""}, {18, "aoa"}, {77, "Foo"}}
        );
    }

    void changedTwoSeparateItems3() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "666"}, {666, ""}, {1, "uwu"}, {18, "Nope"}, {77, "www"}}
        );
    }

    void removedAndAdded() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{666, ""}, {1, ""}, {18, "Nope"}, {33, "fffu"}, {-2, "ew"}}
        );
    }

    void changedAndAdded() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "666"}, {666, "yeeee"}, {1, "what"}, {18, "Nope"}, {77, "now"}, {999, "big"}}
        );
    }

    void removedChangedAndAdded() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "sad"}, {1, "small"}, {33, "fffu"}, {-2, "ew"}}
        );
    }

    void removedChangedAndAdded2() {
        checkUpdate(
            {{42, "666"}, {666, ""}, {1, ""}, {18, "Nope"}, {77, "Foo"}},
            {{42, "666"}, {666, "333"}, {18, "Nope"}, {77, "Foo"}}
        );
    }

private:
    void checkUpdate(const std::vector<Item>& oldList, std::vector<Item> newList) {
        checkThatItemsAreUnique(oldList);
        checkThatItemsAreUnique(newList);

        logInfo("Checking update from {}", oldList);
        std::sort(newList.begin(), newList.end());
        do {
            logInfo(" - to {}", newList);
            try {
                checkUpdateInner(oldList, newList);
            } catch (const AbortTest&) {
                break;
            }
        } while (std::next_permutation(newList.begin(), newList.end()));
    }

    void checkUpdateInner(const std::vector<Item>& oldList, const std::vector<Item>& newList) {
        auto directlyUpdatedList = oldList;
        Updater updater;
        updater.update(directlyUpdatedList, std::vector(newList));

        QVERIFY_THROW(updater.aboutToRemoveIndexRanges == updater.removedIndexRanges);
        QVERIFY_THROW(updater.aboutToAddCount == updater.addedCount);

        QCOMPARE_THROW(directlyUpdatedList.size(), newList.size());
        checkThatItemsAreUnique(directlyUpdatedList);
        QCOMPARE_THROW(
            std::set(directlyUpdatedList.begin(), directlyUpdatedList.end()),
            std::set(newList.begin(), newList.end())
        );

        auto indirectlyUpdatedList = oldList;

        for (const auto& [first, last] : updater.removedIndexRanges) {
            indirectlyUpdatedList.erase(
                indirectlyUpdatedList.begin() + static_cast<ptrdiff_t>(first),
                indirectlyUpdatedList.begin() + static_cast<ptrdiff_t>(last)
            );
        }

        for (const auto& [first, last] : updater.changedIndexRanges) {
            std::copy(
                directlyUpdatedList.begin() + static_cast<ptrdiff_t>(first),
                directlyUpdatedList.begin() + static_cast<ptrdiff_t>(last),
                indirectlyUpdatedList.begin() + static_cast<ptrdiff_t>(first)
            );
        }

        if (updater.addedCount) {
            indirectlyUpdatedList.reserve(indirectlyUpdatedList.size() + *updater.addedCount);
            std::copy(
                directlyUpdatedList.end() - static_cast<ptrdiff_t>(*updater.addedCount),
                directlyUpdatedList.end(),
                std::back_insert_iterator(indirectlyUpdatedList)
            );
        }

        QCOMPARE_THROW(indirectlyUpdatedList, directlyUpdatedList);
    }

    void checkThatItemsAreUnique(const std::vector<Item>& list) {
        const auto set = std::set(list.begin(), list.end());
        QCOMPARE_THROW(set.size(), list.size());
    }
};

QTEST_GUILESS_MAIN(ItemListUpdaterTest)

#include "itemlistupdater_test.moc"
