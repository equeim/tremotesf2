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

#include <QtGlobal>

namespace tremotesf {
    namespace impl {
        template<typename T>
        concept HasReserveFunction = std::ranges::range<T> && requires(T container, std::ranges::range_size_t<T> size) {
            container.reserve(size);
        };

        template<typename T>
        concept HasPushBackFunction =
            std::ranges::range<T> &&
            requires(T container, std::ranges::range_reference_t<T> item) { container.push_back(item); };

        template<typename T>
        concept HasInsertFunction =
            std::ranges::range<T> &&
            requires(T container, std::ranges::range_reference_t<T> item) { container.insert(item); };

        template<typename T>
        concept MovableRange = (std::is_rvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>) ||
                               (!std::is_reference_v<T> && !std::is_const_v<T>);

        template<std::ranges::range T>
        using MaybeMovableRangeReference = std::
            conditional_t<MovableRange<T>, std::ranges::range_rvalue_reference_t<T>, std::ranges::range_reference_t<T>>;
    }

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

    template<
        std::default_initializable NewContainer,
        std::ranges::forward_range FromRange,
        std::invocable<impl::MaybeMovableRangeReference<FromRange>> Transform>
        requires(impl::HasPushBackFunction<NewContainer> || impl::HasInsertFunction<NewContainer>)
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
    inline NewContainer createTransforming(FromRange&& from, Transform&& transform) {
        NewContainer container{};
        if constexpr (std::ranges::sized_range<FromRange> && impl::HasReserveFunction<NewContainer>) {
            container.reserve(static_cast<std::ranges::range_size_t<NewContainer>>(std::ranges::size(from)));
        }
        auto outputIterator = [&] {
            if constexpr (impl::HasPushBackFunction<NewContainer>) {
                return std::back_insert_iterator(container);
            } else if constexpr (impl::HasInsertFunction<NewContainer>) {
                return std::insert_iterator(container);
            }
        }();
        if constexpr (impl::MovableRange<FromRange>) {
            std::transform(
                std::move_iterator(std::ranges::begin(from)),
                std::move_iterator(std::ranges::end(from)),
                outputIterator,
                std::forward<Transform>(transform)
            );
        } else {
            std::transform(
                std::ranges::begin(from),
                std::ranges::end(from),
                outputIterator,
                std::forward<Transform>(transform)
            );
        }
        return container;
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
