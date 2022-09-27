// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_BENCODEPARSER_H
#define TREMOTESF_BENCODEPARSER_H

#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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

    template<typename ValueType>
    inline constexpr const char* getValueTypeName() {
        using T = std::decay_t<ValueType>;
        if constexpr (std::is_same_v<T, Integer>) {
            return "Integer";
        } else if constexpr (std::is_same_v<T, ByteArray>) {
            return "ByteArray";
        } else if constexpr (std::is_same_v<T, List>) {
            return "List";
        } else if constexpr (std::is_same_v<T, Dictionary>) {
            return "Dictionary";
        } else if constexpr (std::is_same_v<T, QString>) {
            return "String";
        } else {
            static_assert(!std::is_same_v<T, T>, "Unsupported type");
        }
    }

    class Value {
    public:
        Value() = default;
        inline Value(Integer value) : mValue{value} {}
        inline Value(ByteArray&& value) : mValue{std::move(value)} {}
        inline Value(List&& value) : mValue{std::move(value)} {}
        inline Value(Dictionary&& value) : mValue{std::move(value)} {}

        inline std::optional<Integer> maybeTakeInteger();
        inline std::optional<ByteArray> maybeTakeByteArray();
        inline std::optional<QString> maybeTakeString();
        inline std::optional<List> maybeTakeList();
        inline std::optional<Dictionary> maybeTakeDictionary();

        inline Integer takeInteger();
        inline ByteArray takeByteArray();
        inline QString takeString();
        inline List takeList();
        inline Dictionary takeDictionary();

        template<typename Expected>
        std::optional<Expected> maybeTakeValue();

        template<typename Expected>
        Expected takeValue();
    private:
        using Variant = std::variant<Integer, ByteArray, List, Dictionary>;
        Variant mValue{};
    };

    extern template std::optional<Integer> Value::maybeTakeValue();
    extern template std::optional<ByteArray> Value::maybeTakeValue();
    extern template std::optional<List> Value::maybeTakeValue();
    extern template std::optional<Dictionary> Value::maybeTakeValue();

    template<>
    std::optional<QString> Value::maybeTakeValue();
    extern template std::optional<QString> Value::maybeTakeValue();

    inline std::optional<Integer> Value::maybeTakeInteger() { return maybeTakeValue<Integer>(); }
    inline std::optional<ByteArray> Value::maybeTakeByteArray() { return maybeTakeValue<ByteArray>(); }
    inline std::optional<QString> Value::maybeTakeString() { return maybeTakeValue<QString>(); }
    inline std::optional<List> Value::maybeTakeList() { return maybeTakeValue<List>(); }
    inline std::optional<Dictionary> Value::maybeTakeDictionary() { return maybeTakeValue<Dictionary>(); }

    extern template Integer Value::takeValue();
    extern template ByteArray Value::takeValue();
    extern template List Value::takeValue();
    extern template Dictionary Value::takeValue();
    extern template QString Value::takeValue();

    inline Integer Value::takeInteger() { return takeValue<Integer>(); }
    inline ByteArray Value::takeByteArray() { return takeValue<ByteArray>(); }
    inline QString Value::takeString() { return takeValue<QString>(); }
    inline List Value::takeList() { return takeValue<List>(); }
    inline Dictionary Value::takeDictionary() { return takeValue<Dictionary>(); }

    class Error : public std::runtime_error {
    public:
        enum class Type
        {
            Reading,
            Parsing
        };

        inline explicit Error(Type type, const char* what) : std::runtime_error(what), mType(type) {}
        inline explicit Error(Type type, const std::string& what) : std::runtime_error(what), mType(type) {}

        inline Type type() const { return mType; }

    private:
        Type mType;
    };

    Value parse(const QString& filePath);
    Value parse(QIODevice& device);
}
}

#endif // TREMOTESF_BENCODEPARSER_H
