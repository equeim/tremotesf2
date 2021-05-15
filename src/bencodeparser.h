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

#ifndef TREMOTESF_BENCODEPARSER_H
#define TREMOTESF_BENCODEPARSER_H

#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <QString>

class QIODevice;

namespace tremotesf
{
namespace bencode
{
    class Value;

    using Integer = int64_t;
    using ByteArray = std::string;
    using List = std::list<Value>;
    struct DictionaryComparator
    {
        using is_transparent = void;
        inline bool operator()(const ByteArray& key1, const ByteArray& key2) const { return key1 < key2; }
        inline bool operator()(std::string_view key1, const ByteArray& key2) const { return key1 < key2; }
        inline bool operator()(const ByteArray& key1, std::string_view key2) const { return key1 < key2; }
    };
    using Dictionary = std::map<ByteArray, Value, DictionaryComparator>;

    class Value {
    public:
        Value() = default;
        inline Value(Integer value) : mValue{value} {}
        inline Value(ByteArray&& value) : mValue{std::move(value)} {}
        inline Value(List&& value) : mValue{std::move(value)} {}
        inline Value(Dictionary&& value) : mValue{std::move(value)} {}

        inline std::optional<Integer> takeInteger();
        inline std::optional<ByteArray> takeByteArray();
        inline std::optional<QString> takeString();
        inline std::optional<List> takeList();
        inline std::optional<Dictionary> takeDictionary();

        template<typename Expected>
        std::optional<Expected> takeValue();
    private:
        using Variant = std::variant<Integer, ByteArray, List, Dictionary>;
        Variant mValue{};
    };

    extern template std::optional<Integer> Value::takeValue();
    extern template std::optional<ByteArray> Value::takeValue();
    extern template std::optional<List> Value::takeValue();
    extern template std::optional<Dictionary> Value::takeValue();

    template<>
    std::optional<QString> Value::takeValue();
    extern template std::optional<QString> Value::takeValue();

    inline std::optional<Integer> Value::takeInteger() { return takeValue<Integer>(); }
    inline std::optional<ByteArray> Value::takeByteArray() { return takeValue<ByteArray>(); }
    inline std::optional<QString> Value::takeString() { return takeValue<QString>(); }
    inline std::optional<List> Value::takeList() { return takeValue<List>(); }
    inline std::optional<Dictionary> Value::takeDictionary() { return takeValue<Dictionary>(); }

    enum Error
    {
        NoError,
        ReadingError,
        ParsingError
    };

    struct Result
    {
        Value parseResult;
        Error error;
    };

    Result parse(const QString& filePath);
    Result parse(QIODevice& device);
}
}

#endif // TREMOTESF_BENCODEPARSER_H
