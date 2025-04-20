// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LOG_LOG_H
#define TREMOTESF_LOG_LOG_H

#include <concepts>
#include <type_traits>
#include <string>
#include <source_location>

#include <QLoggingCategory>
#include <QMessageLogger>
#include <QString>

#if FMT_VERSION_MAJOR >= 11
#    include <fmt/base.h>
#else
#    include <fmt/core.h>
#endif

#ifdef Q_OS_WIN
#    include <guiddef.h>
#    include <winrt/base.h>
#endif

#include "formatters.h"

#if __has_cpp_attribute(gnu::always_inline)
#    define ALWAYS_INLINE [[gnu::always_inline]] inline
#elif __has_cpp_attribute(msvc::forceinline)
#    define ALWAYS_INLINE [[msvc::forceinline]] inline
#elifdef _MSC_VER
#    define ALWAYS_INLINE __forceinline
#else
#    define ALWAYS_INLINE inline
#endif

namespace tremotesf {
    namespace impl {
        template<typename T>
        concept IsException = std::derived_from<std::remove_cvref_t<T>, std::exception>
#ifdef Q_OS_WIN
                              || std::derived_from<std::remove_cvref_t<T>, winrt::hresult_error>
#endif
            ;

        template<typename T>
        concept IsQStringView = std::same_as<std::remove_cvref_t<T>, QStringView>
                                || std::same_as<std::remove_cvref_t<T>, QUtf8StringView>
                                || std::same_as<std::remove_cvref_t<T>, QAnyStringView>;

        template<std::convertible_to<QString> T>
        ALWAYS_INLINE QString convertToQString(const T& string) {
            return static_cast<QString>(string);
        }

        template<std::convertible_to<std::string_view> T>
            requires(!std::convertible_to<T, QString> && !impl::IsQStringView<T>)
        ALWAYS_INLINE QString convertToQString(const T& string) {
            const auto stringView = static_cast<std::string_view>(string);
            return QString::fromUtf8(stringView.data(), static_cast<QString::size_type>(stringView.size()));
        }

        template<impl::IsQStringView T>
        ALWAYS_INLINE QString convertToQString(const T& string) {
            return string.toString();
        }

        template<typename T>
        concept CanConvertToQString = !std::same_as<std::remove_cvref_t<T>, QString>
                                      && requires(T string) { tremotesf::impl::convertToQString(string); };

        inline void printNewline(FILE* stream) { std::fwrite("\n", 1, 1, stream); }
    }

    constexpr auto tremotesfLoggingCategoryName = "tremotesf";
    Q_DECLARE_LOGGING_CATEGORY(tremotesfLoggingCategory)

    void overrideDebugLogs(bool enable);

    std::string formatExceptionRecursively(const std::exception& e);
    std::string formatExceptionRecursively(const std::system_error& e);
#ifdef Q_OS_WIN
    std::string formatExceptionRecursively(const winrt::hresult_error& e);
#endif

    struct Logger {
        ALWAYS_INLINE consteval explicit Logger(QtMsgType type, std::source_location location)
            : type(type),
              context(
                  location.file_name(),
                  static_cast<int>(location.line()),
                  location.function_name(),
                  tremotesfLoggingCategoryName
              ) {}

        ALWAYS_INLINE void log(const QString& string) const {
            if (isEnabled()) {
                logImpl(string);
            }
        }

        template<impl::CanConvertToQString T>
        ALWAYS_INLINE void log(const T& string) const {
            if (isEnabled()) {
                logImpl(impl::convertToQString(string));
            }
        }

        /**
         * Format then print
         */

        template<typename T>
        ALWAYS_INLINE void log(const T& value) const {
            if (isEnabled()) {
                logWithFormatArgs(
                    fmt::format_string<const T&>(impl::singleArgumentFormatString),
                    fmt::make_format_args(value)
                );
            }
        }

        template<typename... Args>
            requires(sizeof...(Args) != 0)
        ALWAYS_INLINE void log(fmt::format_string<Args...> fmt, const Args&... args) const {
            if (isEnabled()) {
                logWithFormatArgs(fmt, fmt::make_format_args(args...));
            }
        }

        /**
         * Special functions to print nested exceptions recursively
         */

        template<impl::IsException E, typename T>
        ALWAYS_INLINE void logWithException(const E& e, const T& value) const {
            if (isEnabled()) {
                const auto formattedException = formatExceptionRecursively(e);
                logWithFormatArgs(
                    fmt::format_string<const T&, const std::string&>("{}\n{}"),
                    fmt::make_format_args(value, formattedException)
                );
            }
        }

        template<impl::IsException E, typename... Args>
            requires(sizeof...(Args) != 0)
        ALWAYS_INLINE void logWithException(const E& e, fmt::format_string<Args...> fmt, const Args&... args) const {
            if (isEnabled()) {
                auto message = formatToQString(fmt, fmt::make_format_args(args...));
                message += '\n';
                message += formatExceptionRecursively(e);
                logImpl(message);
            }
        }

    private:
        ALWAYS_INLINE bool isEnabled() const { return tremotesfLoggingCategory().isEnabled(type); }

        static QString formatToQString(fmt::string_view fmt, fmt::format_args args);
        void logWithFormatArgs(fmt::string_view fmt, fmt::format_args args) const;

        /**
         * Actual log function
         */
        void logImpl(const QString& string) const;

        QtMsgType type;
        QMessageLogContext context;
    };

    ALWAYS_INLINE consteval Logger debug(std::source_location location = std::source_location::current()) {
        return Logger(QtDebugMsg, location);
    }

    ALWAYS_INLINE consteval Logger info(std::source_location location = std::source_location::current()) {
        return Logger(QtInfoMsg, location);
    }

    ALWAYS_INLINE consteval Logger warning(std::source_location location = std::source_location::current()) {
        return Logger(QtWarningMsg, location);
    }

    ALWAYS_INLINE consteval Logger fatal(std::source_location location = std::source_location::current()) {
        return Logger(QtFatalMsg, location);
    }

    template<typename T>
    ALWAYS_INLINE void printlnStdout(const T& value) {
        fmt::print(stdout, fmt::format_string<const T&>(impl::singleArgumentFormatString), value);
        impl::printNewline(stdout);
    }

    template<typename... Args>
        requires(sizeof...(Args) != 0)
    ALWAYS_INLINE void printlnStdout(fmt::format_string<Args...> fmt, const Args&... args) {
        fmt::vprint(stdout, fmt, fmt::make_format_args(args...));
        impl::printNewline(stdout);
    }
}

#endif // TREMOTESF_LOG_LOG_H
