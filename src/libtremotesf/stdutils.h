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

#ifndef TREMOTESF_STDUTILS_H
#define TREMOTESF_STDUTILS_H

#include <functional>
#include <iterator>
#include <type_traits>
#include <vector>

#include <QtGlobal>

namespace libtremotesf
{
    template<typename C, typename V>
    inline auto contains_impl(const C& container, const V& value, int) -> decltype(container.find(value), true)
    {
        return container.find(value) != std::end(container);
    }

    template<typename C, typename V>
    inline bool contains_impl(const C& container, const V& value, long)
    {
        return std::find(std::begin(container), std::end(container), value) != std::end(container);
    }

    template<typename C, typename V>
    inline bool contains(const C& container, const V& value)
    {
        return contains_impl(container, value, 0);
    }

    template<typename C, typename V>
    inline size_t index_of(const C& container, const V& value) {
        return static_cast<size_t>(std::find(std::begin(container), std::end(container), value) - std::begin(container));
    }

    template<typename C, typename V>
    inline int index_of_i(const C& container, const V& value) {
        return static_cast<int>(std::find(std::begin(container), std::end(container), value) - std::begin(container));
    }

    template<typename C, typename V>
    inline void erase_one(C& container, const V& value) {
        container.erase(std::find(container.begin(), container.end(), value));
    }


    template<typename T, typename std::enable_if<std::is_scalar<T>::value && !std::is_floating_point<T>::value, int>::type = 0>
    inline void setChanged(T& value, T newValue, bool& changed)
    {
        if (newValue != value) {
            value = newValue;
            changed = true;
        }
    }

    template<typename T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline void setChanged(T& value, T newValue, bool& changed)
    {
        if (!qFuzzyCompare(newValue, value)) {
            value = newValue;
            changed = true;
        }
    }

    template<typename T, typename std::enable_if<!std::is_scalar<T>::value, int>::type = 0>
    inline void setChanged(T& value, T&& newValue, bool& changed)
    {
        if (newValue != value) {
            value = std::forward<T>(newValue);
            changed = true;
        }
    }

    template<typename T>
    class VectorBatchRemover
    {
    public:
        inline explicit VectorBatchRemover(std::vector<T>& items, std::vector<int>* removedIndexes = nullptr, std::vector<int>* indexesToShift = nullptr)
            : items(items), removedIndexes(removedIndexes), indexesToShift(indexesToShift) {}

        inline void remove(int index) {
            if (removedIndexes) {
                removedIndexes->push_back(index);
            }
            if (beginIndex == -1) {
                reset(index);
            } else {
                if (index == (beginIndex - 1)) {
                    beginIndex = index;
                } else {
                    doRemove();
                    reset(index);
                }
            }
        }

        inline void doRemove() {
            if (beginIndex != -1) {
                items.erase(begin + beginIndex, begin + endIndex + 1);
                if (indexesToShift && !indexesToShift->empty()) {
                    const int shift = static_cast<int>(endIndex - beginIndex + 1);
                    for (int& index : *indexesToShift) {
                        if (index < beginIndex) {
                            break;
                        }
                        index -= shift;
                    }
                }
            }
        }

    private:
        inline void reset(int index) {
            beginIndex = index;
            endIndex = index;
        }

        std::vector<T>& items;
        std::vector<int>* const removedIndexes;
        std::vector<int>* const indexesToShift;

        const typename std::vector<T>::iterator begin = items.begin();

        int beginIndex = -1;
        int endIndex = -1;
    };
}

namespace tremotesf
{
    using libtremotesf::contains;
    using libtremotesf::index_of;
    using libtremotesf::index_of_i;
    using libtremotesf::erase_one;
    using libtremotesf::setChanged;
    using libtremotesf::VectorBatchRemover;
}

#endif // TREMOTESF_STDUTILS_H
