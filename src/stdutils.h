// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_STDUTILS_H
#define TREMOTESF_STDUTILS_H

#include <algorithm>
#include <concepts>
#include <optional>
#include <ranges>
#include <type_traits>

#include <QtNumeric>

namespace tremotesf {
    template<std::ranges::random_access_range Range>
    inline constexpr std::optional<std::ranges::range_size_t<Range>>
    indexOf(const Range& range, std::ranges::range_value_t<Range> value) {
        namespace r = std::ranges;
        const auto found = r::find(range, value);
        if (found == r::end(range)) {
            return std::nullopt;
        };
        return static_cast<r::range_size_t<Range>>(r::distance(r::begin(range), found));
    }

    template<std::integral Index, std::ranges::random_access_range Range>
    inline constexpr std::optional<Index> indexOfCasted(const Range& range, std::ranges::range_value_t<Range> value) {
        const auto index = indexOf(range, value);
        if (!index.has_value()) return std::nullopt;
        return static_cast<Index>(*index);
    }

    template<std::ranges::random_access_range Range, std::integral Index>
    inline constexpr auto slice(const Range& range, Index first, Index last) {
        const auto begin = std::ranges::begin(range);
        return std::ranges::subrange(
            begin + static_cast<std::ranges::range_difference_t<Range>>(first),
            begin + static_cast<std::ranges::range_difference_t<Range>>(last)
        );
    }

    /**
     * T1 is always deduced as value type, T2 may be a reference
     */
    template<std::regular T1, typename T2 = T1>
        requires(!std::floating_point<T1> && std::same_as<std::remove_const_t<T1>, std::remove_cvref_t<T2>>)
    inline void setChanged(T1& value, T2&& newValue, bool& changed) {
        if (newValue != value) {
            value = std::forward<T2>(newValue);
            changed = true;
        }
    }

    template<std::floating_point T>
    inline void setChanged(T& value, T newValue, bool& changed) {
        if (!qFuzzyCompare(newValue, value)) {
            value = newValue;
            changed = true;
        }
    }
}

#endif // TREMOTESF_STDUTILS_H
