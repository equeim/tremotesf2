// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bencodeparser.h"

#include <array>
#include <charconv>
#include <cinttypes>
#include <limits>
#include <system_error>

#include <QFile>
#include <QString>

#include "libtremotesf/formatters.h"

namespace tremotesf::bencode
{
    namespace {
        constexpr char integerPrefix = 'i';
        constexpr char listPrefix = 'l';
        constexpr char dictionaryPrefix = 'd';
        constexpr char terminator = 'e';
        constexpr char byteArraySeparator = ':';

        // digits10 + 1 is maximum number of character needed to hold integer, +1 for minus sign, +1 for terminator character
        constexpr int integerBufferSize = std::numeric_limits<Integer>::digits10 + 3;
    }

    template<typename Expected>
    std::optional<Expected> Value::maybeTakeValue()
    {
        return std::visit([](auto&& value) -> std::optional<Expected> {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Expected>) {
                return std::move(value);
            } else {
                return std::nullopt;
            }
        }, mValue);
    }

    template std::optional<Integer> Value::maybeTakeValue();
    template std::optional<ByteArray> Value::maybeTakeValue();
    template std::optional<List> Value::maybeTakeValue();
    template std::optional<Dictionary> Value::maybeTakeValue();

    template<>
    std::optional<QString> Value::maybeTakeValue()
    {
        return std::visit([](auto&& value) -> std::optional<QString> {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, ByteArray>) {
                auto string = QString::fromStdString(value);
                value.clear();
                return string;
            } else {
                return std::nullopt;
            }
        }, mValue);
    }

    template<typename Expected>
    Expected Value::takeValue()
    {
        if (auto maybeValue = maybeTakeValue<Expected>(); maybeValue) {
            return std::move(*maybeValue);
        }
        throw Error(Error::Type::Parsing, std::string("Value is not of ") + getValueTypeName<Expected>() + " type");
    }

    template Integer Value::takeValue();
    template ByteArray Value::takeValue();
    template List Value::takeValue();
    template Dictionary Value::takeValue();
    template QString Value::takeValue();

    namespace
    {
        template<bool IsSequential>
        class Parser
        {
        public:
            explicit Parser(QIODevice& device) : mDevice{device} {};

            Value parse()
            {
                return parseValue();
            }

        private:
            Value parseValue()
            {
                const char byte = peekByte();
                if (byte == integerPrefix) {
                    return parseInteger();
                }
                if (byte == listPrefix) {
                    return parseList();
                }
                if (byte == dictionaryPrefix) {
                    return parseDictionary();
                }
                return parseByteArray();
            }

            Dictionary parseDictionary()
            {
                return parseContainer<Dictionary>([&](auto& dict) {
                    ByteArray key(parseByteArray());
                    Value value(parseValue());
                    dict.emplace(std::move(key), std::move(value));
                });
            }

            List parseList()
            {
                return parseContainer<List>([&](auto& list) {
                    list.push_back(parseValue());
                });
            }

            template<typename Container, typename Append>
            Container parseContainer(Append&& append)
            {
                const auto containerPos = mDevice.pos();
                try {
                    skipByte();
                    Container container{};
                    while (true) {
                        if (peekByte() == terminator) {
                            skipByte();
                            return container;
                        }
                        append(container);
                    }
                } catch (const Error& e) {
                    std::throw_with_nested(Error(e.type(), fmt::format("Failed to parse {} at position {}", getValueTypeName<Container>(), containerPos)));
                }
                throw Error(Error::Type::Parsing, fmt::format("Failed to parse {} at position {}", getValueTypeName<Container>(), containerPos));
            }

            ByteArray parseByteArray()
            {
                const auto byteArrayPos = mDevice.pos();
                try {
                    const auto size = readIntegerUntilTerminator(byteArraySeparator);
                    if (size < 0) {
                        throw Error(Error::Type::Parsing, fmt::format("Incorrect byte array size {}", size));
                    }
                    ByteArray byteArray(static_cast<size_t>(size), 0);
                    const auto read = mDevice.read(byteArray.data(), size);
                    if (read != size) {
                        throwErrorFromIODevice(fmt::format("Failed to read byte array with size {} (read {} bytes)", size, read));
                    }
                    return byteArray;
                } catch (const Error& e) {
                    std::throw_with_nested(Error(e.type(), fmt::format("Failed to parse byte array at position {}", byteArrayPos)));
                }
            }

            Integer parseInteger()
            {
                const auto integerPos = mDevice.pos();
                try {
                    skipByte();
                    return readIntegerUntilTerminator(terminator);
                } catch (const Error& e) {
                    std::throw_with_nested(Error(e.type(), fmt::format("Failed to parse integer at position {}", integerPos)));
                }
            }

            Integer readIntegerUntilTerminator(char integerTerminator) {

                const auto peeked = mDevice.peek(mIntegerBuffer.data(), integerBufferSize);
                if (peeked <= 0) {
                    throwErrorFromIODevice("Failed to peek integer buffer");
                }
                Integer integer{};
                const auto result = std::from_chars(mIntegerBuffer.data(), mIntegerBuffer.data() + integerBufferSize, integer);
                if (result.ec != std::errc{}) {
                    throw Error(
                        Error::Type::Parsing,
                        fmt::format(
                            "std::from_chars() failed with: {} (error code {} ({:#x}))",
                            std::make_error_condition(result.ec).message(),
                            static_cast<std::underlying_type_t<std::errc>>(result.ec),
                            static_cast<std::make_unsigned_t<std::underlying_type_t<std::errc>>>(result.ec)
                        )
                    );
                }
                if (*result.ptr != integerTerminator) {
                    throw Error(Error::Type::Parsing, fmt::format("Terminator doesn't match: expected {}, actual {}", integerTerminator, *result.ptr));
                }
                skip(result.ptr - mIntegerBuffer.data() + 1);
                return integer;
            }

            char peekByte()
            {
                char byte{};
                if (mDevice.peek(&byte, 1) != 1) {
                    throwErrorFromIODevice("Failed to peek 1 byte");
                }
                return byte;
            }

            void skipByte()
            {
                return skip(1);
            }

            void skip(qint64 size)
            {
                if (mDevice.skip(size) != size) {
                    throwErrorFromIODevice(fmt::format("Failed to skip {} bytes", size));
                }
            }

            void throwErrorFromIODevice(std::string&& message)
            {
                Error::Type type{};
                if (mDevice.atEnd()) {
                    type = Error::Type::Parsing;
                } else {
                    type = Error::Type::Reading;
                }
                throw Error(type, fmt::format("{}: {}", std::move(message), mDevice.errorString()));
            }

            QIODevice& mDevice;
            std::array<char, integerBufferSize> mIntegerBuffer{};
        };
    }

    Value parse(const QString& filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            throw Error(Error::Type::Reading, fmt::format("Failed to open file {}: {}", filePath, file.errorString()));
        }
        return parse(file);
    }

    Value parse(QIODevice& device)
    {
        if (device.isSequential()) {
            return Parser<true>(device).parse();
        }
        return Parser<false>(device).parse();
    }
}
