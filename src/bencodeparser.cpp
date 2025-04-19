// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bencodeparser.h"

#include <array>
#include <charconv>
#include <limits>
#include <system_error>

#include <QFile>
#include <QString>

#include <fmt/format.h>

#include "fileutils.h"

namespace tremotesf::bencode {
    namespace {
        constexpr char integerPrefix = 'i';
        constexpr char listPrefix = 'l';
        constexpr char dictionaryPrefix = 'd';
        constexpr char terminator = 'e';
        constexpr char byteArraySeparator = ':';

        // digits10 + 1 is maximum number of character needed to hold integer, +1 for minus sign, +1 for terminator character
        constexpr int integerBufferSize = std::numeric_limits<Integer>::digits10 + 3;
    }

    template<ValueType Expected>
    Expected Value::takeValue() && {
        return std::visit(
            [](auto&& value) -> Expected {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::same_as<T, Expected>) {
                    return std::forward<Expected>(value);
                } else if constexpr (std::same_as<T, ByteArray> && std::same_as<Expected, QString>) {
                    auto string = QString::fromStdString(value);
                    value.clear();
                    return string;
                } else {
                    throw Error(
                        Error::Type::Parsing,
                        fmt::format("Value is not of {} type", getValueTypeName<Expected>())
                    );
                }
            },
            std::move(mValue)
        );
    }

    template Integer Value::takeValue() &&;
    template ByteArray Value::takeValue() &&;
    template List Value::takeValue() &&;
    template Dictionary Value::takeValue() &&;
    template QString Value::takeValue() &&;

    namespace {
        class Parser {
        public:
            explicit Parser(QFile& file, ReadRootDictionaryValueCallback onReadRootDictionaryValue)
                : mFile{file}, mOnReadRootDictionaryValue(std::move(onReadRootDictionaryValue)) {};

            Value parse() { return parseValue(true); }

        private:
            Value parseValue(bool root = false) {
                const char byte = peekByte();
                if (byte == integerPrefix) {
                    return parseInteger();
                }
                if (byte == listPrefix) {
                    return parseList();
                }
                if (byte == dictionaryPrefix) {
                    return parseDictionary(root);
                }
                return parseByteArray();
            }

            Dictionary parseDictionary(bool root) {
                return parseContainer<Dictionary>([&](Dictionary& dict) {
                    ByteArray key(parseByteArray());
                    const auto valuePos = mFile.pos();
                    Value value(parseValue());
                    if (root) {
                        mOnReadRootDictionaryValue(key, valuePos, mFile.pos() - valuePos);
                    }
                    dict.emplace(std::move(key), std::move(value));
                });
            }

            List parseList() {
                return parseContainer<List>([&](List& list) { list.push_back(parseValue()); });
            }

            template<std::default_initializable Container, std::invocable<Container&> ParseNextElement>
            Container parseContainer(ParseNextElement parseNextElement) {
                const auto containerPos = mFile.pos();
                try {
                    skipByte();
                    Container container{};
                    while (true) {
                        if (peekByte() == terminator) {
                            skipByte();
                            return container;
                        }
                        parseNextElement(container);
                    }
                } catch (const Error& e) {
                    std::throw_with_nested(Error(
                        e.type(),
                        fmt::format("Failed to parse {} at position {}", getValueTypeName<Container>(), containerPos)
                    ));
                }
            }

            ByteArray parseByteArray() {
                const auto byteArrayPos = mFile.pos();
                try {
                    Integer size{};
                    try {
                        size = readIntegerUntilTerminator(byteArraySeparator);
                    } catch (const Error& e) {
                        std::throw_with_nested(Error(e.type(), "Failed to parse byte array size"));
                    }
                    if (size < 0) {
                        throw Error(Error::Type::Parsing, fmt::format("Incorrect byte array size {}", size));
                    }
                    ByteArray byteArray(static_cast<size_t>(size), 0);
                    if (size != 0) {
                        try {
                            readBytes(mFile, byteArray);
                        } catch (const QFileError&) {
                            std::throw_with_nested(Error(
                                Error::Type::Reading,
                                fmt::format("Failed to read byte array data with size {}", size)
                            ));
                        }
                    }
                    return byteArray;
                } catch (const Error& e) {
                    std::throw_with_nested(
                        Error(e.type(), fmt::format("Failed to parse byte array at position {}", byteArrayPos))
                    );
                }
            }

            Integer parseInteger() {
                const auto integerPos = mFile.pos();
                try {
                    skipByte();
                    return readIntegerUntilTerminator(terminator);
                } catch (const Error& e) {
                    std::throw_with_nested(
                        Error(e.type(), fmt::format("Failed to parse integer at position {}", integerPos))
                    );
                }
            }

            Integer readIntegerUntilTerminator(char integerTerminator) {
                std::span<const char> peeked{};
                try {
                    peeked = peekBytes(mFile, mIntegerBuffer);
                } catch (const QFileError&) {
                    std::throw_with_nested(Error(Error::Type::Reading, "Failed to read integer buffer"));
                }

                Integer integer{};

                const auto result =
                    std::from_chars(std::to_address(peeked.begin()), std::to_address(peeked.end()), integer);
                if (result.ec != std::errc{}) {
                    throw Error(
                        Error::Type::Parsing,
                        fmt::format(
                            "std::from_chars() failed with: {} (error code {} ({:#x}))",
                            std::make_error_condition(result.ec).message(),
                            std::to_underlying(result.ec),
                            static_cast<std::make_unsigned_t<std::underlying_type_t<std::errc>>>(result.ec)
                        )
                    );
                }
                if (result.ptr == std::to_address(peeked.end())) {
                    throw Error(
                        Error::Type::Parsing,
                        fmt::format(
                            "Didn't find integer terminator \"{}\", file is possibly truncated",
                            integerTerminator
                        )
                    );
                }
                if (*result.ptr != integerTerminator) {
                    throw Error(
                        Error::Type::Parsing,
                        fmt::format(
                            R"(Integer terminator doesn't match: expected "{}", actual "{}")",
                            integerTerminator,
                            *result.ptr
                        )
                    );
                }
                const auto terminatorIndex = result.ptr - peeked.data();
                skip(terminatorIndex + 1);
                return integer;
            }

            char peekByte() {
                std::array<char, 1> buffer{};
                try {
                    [[maybe_unused]] const auto peeked = peekBytes(mFile, buffer);
                } catch (const QFileError&) {
                    std::throw_with_nested(Error(Error::Type::Reading, "Failed to peek byte"));
                }
                return buffer[0];
            }

            void skipByte() { skip(1); }

            void skip(qint64 size) {
                try {
                    skipBytes(mFile, size);
                } catch (const QFileError&) {
                    std::throw_with_nested(Error(Error::Type::Reading, fmt::format("Failed to skip {} bytes", size)));
                }
            }

            QFile& mFile;
            ReadRootDictionaryValueCallback mOnReadRootDictionaryValue;
            std::array<char, integerBufferSize> mIntegerBuffer{};
        };
    }

    Value parse(const QString& filePath, ReadRootDictionaryValueCallback onReadRootDictionaryValue) {
        QFile file(filePath);
        try {
            openFile(file, QIODevice::ReadOnly);
        } catch (const QFileError&) {
            std::throw_with_nested(Error(Error::Type::Reading, "Failed to open file"));
        }
        return parse(file, std::move(onReadRootDictionaryValue));
    }

    Value parse(QFile& device, ReadRootDictionaryValueCallback onReadRootDictionaryValue) {
        return Parser(device, std::move(onReadRootDictionaryValue)).parse();
    }
}
