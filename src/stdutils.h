// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_STDUTILS_H
#define TREMOTESF_STDUTILS_H

#include <algorithm>
#include <concepts>
#include <optional>
#include <ranges>
#include <type_traits>

#if __has_include(<QtNumeric>)
#    include <QtNumeric>
#else
#    include <QtGlobal>
#endif

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

    namespace impl {
        template<typename NewContainer, typename FromRange>
        inline NewContainer toContainer(FromRange&& from) {
            if constexpr (std::ranges::common_range<FromRange>) {
                return {std::ranges::begin(from), std::ranges::end(from)};
            } else {
                auto common = std::views::common(std::forward<FromRange>(from));
                return {std::ranges::begin(common), std::ranges::end(common)};
            }
        }

        template<typename NewContainer, typename FromRange>
        inline NewContainer moveToContainer(FromRange&& from) {
            if constexpr (std::ranges::common_range<FromRange>) {
                return {std::move_iterator(std::ranges::begin(from)), std::move_iterator(std::ranges::end(from))};
            } else {
                auto common = std::views::common(std::forward<FromRange>(from));
                return {std::move_iterator(std::ranges::begin(common)), std::move_iterator(std::ranges::end(common))};
            }
        }
    }

    template<template<typename...> typename NewContainer, std::ranges::input_range FromRange>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    inline auto toContainer(FromRange&& from) {
        return impl::toContainer<NewContainer<std::ranges::range_value_t<FromRange>>, FromRange>(
            std::forward<FromRange>(from)
        );
    }

    template<typename NewContainer, std::ranges::input_range FromRange>
        requires(std::ranges::common_range<FromRange>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    inline auto toContainer(FromRange&& from) {
        return impl::toContainer<NewContainer, FromRange>(std::forward<FromRange>(from));
    }

    template<template<typename...> typename NewContainer, std::ranges::forward_range FromRange>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    inline auto moveToContainer(FromRange&& from) {
        return impl::moveToContainer<NewContainer<std::ranges::range_value_t<FromRange>>, FromRange>(
            std::forward<FromRange>(from)
        );
    }

    template<typename NewContainer, std::ranges::forward_range FromRange>
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    inline auto moveToContainer(FromRange&& from) {
        return impl::moveToContainer<NewContainer, FromRange>(std::forward<FromRange>(from));
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
