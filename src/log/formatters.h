// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LOG_FORMATTERS_H
#define TREMOTESF_LOG_FORMATTERS_H

#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <system_error>
#include <type_traits>

#include <QDebug>
#include <QMetaEnum>
#include <QObject>
#include <QString>
#if QT_VERSION_MAJOR >= 6
#    include <QUtf8StringView>
#endif

#if __has_include(<fmt/base.h>)
#    include <fmt/base.h>
#else
#    include <fmt/core.h>
#endif

namespace tremotesf {
    struct SimpleFormatter {
        constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
    };
}

template<>
struct fmt::formatter<QString> : formatter<string_view> {
    format_context::iterator format(const QString& string, format_context& ctx) const;
};

class QStringView;
template<>
struct fmt::formatter<QStringView> : formatter<string_view> {
    format_context::iterator format(const QStringView& string, format_context& ctx) const;
};

class QLatin1String;
template<>
struct fmt::formatter<QLatin1String> : formatter<string_view> {
    format_context::iterator format(const QLatin1String& string, format_context& ctx) const;
};

class QByteArray;
template<>
struct fmt::formatter<QByteArray> : formatter<string_view> {
    format_context::iterator format(const QByteArray& array, format_context& ctx) const;
};

#if QT_VERSION_MAJOR >= 6
template<>
struct fmt::formatter<QUtf8StringView> : formatter<string_view> {
    format_context::iterator format(const QUtf8StringView& string, format_context& ctx) const;
};

class QAnyStringView;
template<>
struct fmt::formatter<QAnyStringView> : formatter<QString> {
    format_context::iterator format(const QAnyStringView& string, format_context& ctx) const;
};
#endif

namespace tremotesf::impl {
    inline constexpr auto singleArgumentFormatString = "{}";

    template<typename T>
    concept QDebugPrintable = requires(T t, QDebug d) { d << t; };

    template<QDebugPrintable T>
    struct QDebugFormatter : SimpleFormatter {
        fmt::format_context::iterator format(const T& t, fmt::format_context& ctx) const {
            QString buffer{};
            QDebug stream(&buffer);
            stream.nospace() << t;
            return fmt::format_to(ctx.out(), singleArgumentFormatString, buffer);
        }
    };

    // This relies on private Qt API but it should work at least until Qt 7
    template<typename T>
    concept QEnum = std::is_enum_v<T> && requires { qt_getEnumMetaObject(T{}); };

    fmt::format_context::iterator formatQEnum(const QMetaEnum& meta, std::intmax_t value, fmt::format_context& ctx);
    fmt::format_context::iterator formatQEnum(const QMetaEnum& meta, std::uintmax_t value, fmt::format_context& ctx);
}

namespace fmt {
    template<std::derived_from<QObject> T>
    struct formatter<T> : tremotesf::SimpleFormatter {
        format_context::iterator format(const T& object, format_context& ctx) const {
            QString buffer{};
            QDebug stream(&buffer);
            stream.nospace() << &object;
            return fmt::format_to(ctx.out(), tremotesf::impl::singleArgumentFormatString, buffer);
        }
    };

    template<tremotesf::impl::QEnum T>
    struct formatter<T> : tremotesf::SimpleFormatter {
        fmt::format_context::iterator format(T t, fmt::format_context& ctx) const {
            const auto meta = QMetaEnum::fromType<T>();
            if constexpr (std::signed_integral<std::underlying_type_t<T>>) {
                return tremotesf::impl::formatQEnum(meta, static_cast<std::intmax_t>(t), ctx);
            } else {
                return tremotesf::impl::formatQEnum(meta, static_cast<std::uintmax_t>(t), ctx);
            }
        }
    };
}

#define SPECIALIZE_FORMATTER_FOR_QDEBUG(Class)                                \
    namespace fmt {                                                           \
        template<>                                                            \
        struct formatter<Class> : tremotesf::impl::QDebugFormatter<Class> {}; \
    }

namespace fmt {
    template<>
    struct formatter<std::exception> : tremotesf::SimpleFormatter {
        format_context::iterator format(const std::exception& e, format_context& ctx) const;
    };

    template<std::derived_from<std::exception> T>
        requires(!std::same_as<T, std::exception>)
    struct formatter<T> : formatter<std::exception> {};

    template<>
    struct formatter<std::system_error> : tremotesf::SimpleFormatter {
        format_context::iterator format(const std::system_error& e, format_context& ctx) const;
    };

    template<std::derived_from<std::system_error> T>
    struct formatter<T> : formatter<std::system_error> {};
}

#ifdef Q_OS_WIN
namespace winrt {
    struct hstring;
    struct hresult_error;
}

namespace fmt {
    template<>
    struct formatter<winrt::hstring> : formatter<QString> {
        format_context::iterator format(const winrt::hstring& str, format_context& ctx) const;
    };

    template<>
    struct formatter<winrt::hresult_error> : tremotesf::SimpleFormatter {
        format_context::iterator format(const winrt::hresult_error& e, format_context& ctx) const;
    };
}
#endif

#endif // TREMOTESF_LOG_FORMATTERS_H
