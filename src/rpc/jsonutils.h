// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_JSONUTILS_H
#define TREMOTESF_RPC_JSONUTILS_H

#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include <QJsonArray>
#include <QJsonValue>

#include <fmt/format.h>

#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QJsonValue)

namespace tremotesf::impl {
    template<typename T>
    concept EnumConstant = std::is_enum_v<T>;

    template<typename T>
    concept JsonConstant = std::same_as<T, int> || std::same_as<T, QLatin1String>;

    template<EnumConstant EnumConstantT, JsonConstant JsonConstantT>
    struct EnumMapping {
        constexpr explicit EnumMapping(EnumConstantT enumValue, JsonConstantT jsonValue)
            : enumValue(enumValue), jsonValue(jsonValue) {}
        EnumConstantT enumValue;
        JsonConstantT jsonValue;
    };

    template<EnumConstant EnumConstantT, size_t EnumCount, JsonConstant JsonConstantT>
    struct EnumMapper {
        constexpr explicit EnumMapper(std::array<EnumMapping<EnumConstantT, JsonConstantT>, EnumCount>&& mappings)
            : mappings(std::move(mappings)) {}

        EnumConstantT fromJsonValue(const QJsonValue& value, QLatin1String key) const {
            const auto jsonValue = [&] {
                if constexpr (std::same_as<JsonConstantT, int>) {
                    if (!value.isDouble()) {
                        warning().log("JSON field with key {} and value {} is not a number", key, value);
                        return std::optional<int>{};
                    }
                    return std::optional(value.toInt());
                } else if constexpr (std::same_as<JsonConstantT, QLatin1String>) {
                    if (!value.isString()) {
                        warning().log("JSON field with key {} and value {} is not a string", key, value);
                        return std::optional<QString>{};
                    }
                    return std::optional(value.toString());
                }
            }();
            if (!jsonValue.has_value()) {
                return {};
            }
            const auto found =
                std::ranges::find(mappings, jsonValue, &EnumMapping<EnumConstantT, JsonConstantT>::jsonValue);
            if (found == mappings.end()) {
                warning().log("JSON field with key {} has unknown value {}", key, value);
                return {};
            }
            return found->enumValue;
        }

        JsonConstantT toJsonConstant(EnumConstantT value) const {
            const auto found =
                std::ranges::find(mappings, value, &EnumMapping<EnumConstantT, JsonConstantT>::enumValue);
            if (found == mappings.end()) {
                throw std::logic_error(fmt::format("Unknown enum value {}", value));
            }
            return found->jsonValue;
        }

    private:
        std::array<EnumMapping<EnumConstantT, JsonConstantT>, EnumCount> mappings{};
    };

    template<std::ranges::input_range FromRange>
        requires std::convertible_to<std::ranges::range_value_t<FromRange>, QJsonValue>
    inline QJsonArray toJsonArray(FromRange&& from) {
        QJsonArray array{};
        std::ranges::copy(from, std::back_insert_iterator(array));
        return array;
    }

    inline void updateDateTime(QDateTime& dateTime, const QJsonValue& value, bool& changed) {
        const auto newDateTime = value.toInteger();
        if (newDateTime > 0) {
            if (!dateTime.isValid() || newDateTime != dateTime.toSecsSinceEpoch()) {
                dateTime.setSecsSinceEpoch(newDateTime);
                changed = true;
            }
        } else {
            if (!dateTime.isNull()) {
                dateTime.setDate({});
                dateTime.setTime({});
                changed = true;
            }
        }
    }
}

#endif // TREMOTESF_RPC_JSONUTILS_H
