// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LOG_LOG_H
#define TREMOTESF_LOG_LOG_H

#include <concepts>
#include <type_traits>

#include <QMessageLogger>
#include <QString>
#include <fmt/core.h>

#ifdef Q_OS_WIN
#    include <guiddef.h>
#    include <winrt/base.h>
#endif

#include "formatters.h"

#if FMT_VERSION < 80000
#    define FORMAT_STRING fmt::string_view
#else
#    define FORMAT_STRING fmt::format_string<Args...>
#endif

#if __has_cpp_attribute(gnu::always_inline)
#    define ALWAYS_INLINE [[gnu::always_inline]] inline
#elif __has_cpp_attribute(msvc::forceinline)
#    define ALWAYS_INLINE [[msvc::forceinline]] inline
#elif defined(_MSC_VER)
#    define ALWAYS_INLINE __forceinline
#else
#    define ALWAYS_INLINE inline
#endif

namespace tremotesf {
    namespace impl {
        template<typename T>
        concept IsException = std::derived_from<std::remove_reference_t<T>, std::exception>
#ifdef Q_OS_WIN
                              || std::derived_from<std::remove_reference_t<T>, winrt::hresult_error>
#endif
            ;

        template<typename T>
        concept IsQStringView = std::same_as<std::remove_cvref_t<T>, QStringView>
#if QT_VERSION_MAJOR >= 6
                                || std::same_as<std::remove_cvref_t<T>, QUtf8StringView> ||
                                std::same_as<std::remove_cvref_t<T>, QAnyStringView>
#endif
            ;

        struct QMessageLoggerDelegate {
            constexpr explicit QMessageLoggerDelegate(
                QtMsgType type, const char* fileName, int lineNumber, const char* functionName
            )
                : type(type), context(fileName, lineNumber, functionName, "default") {}

            /**
             * Actual log function
             */
            void log(const QString& string) const;

            /**
             * Shortcuts to print strings without going through fmt
             */

            template<std::convertible_to<QString> T>
            ALWAYS_INLINE void log(const T& string) const {
                log(static_cast<QString>(string));
            }

            template<std::convertible_to<std::string_view> T>
                requires(!std::convertible_to<T, QString>)
            ALWAYS_INLINE void log(const T& string) const {
                const auto stringView = static_cast<std::string_view>(string);
                log(QString::fromUtf8(stringView.data(), static_cast<QString::size_type>(stringView.size())));
            }

            template<IsQStringView T>
            ALWAYS_INLINE void log(const T& string) const {
                log(string.toString());
            }

            /**
             * Format then print
             */

            template<typename T>
            ALWAYS_INLINE void log(const T& value) const {
                log(singleArgumentFormatString, value);
            }

            template<typename... Args>
                requires(sizeof...(Args) != 0)
            ALWAYS_INLINE void log(FORMAT_STRING fmt, Args&&... args) const {
                log(fmt::format(fmt, std::forward<Args>(args)...));
            }

            /**
             * Special function to print nested exceptions recursively
             */

            template<IsException T>
            ALWAYS_INLINE void log(const T& exception) const {
                logExceptionRecursively<false>(exception);
            }

            template<IsException E, typename T>
            ALWAYS_INLINE void logWithException(const E& e, const T& value) const {
                log(value);
                logExceptionRecursively<true>(e);
            }

            template<IsException E, typename... Args>
                requires(sizeof...(Args) != 0)
            ALWAYS_INLINE void logWithException(const E& e, FORMAT_STRING fmt, Args&&... args) const {
                log(fmt, std::forward<Args>(args)...);
                logExceptionRecursively<true>(e);
            }

        private:
            template<bool PrintCausedBy>
            ALWAYS_INLINE void logExceptionRecursively(const std::exception& e) const {
                return logExceptionRecursivelyImpl<std::exception, PrintCausedBy>(e);
            }

            template<bool PrintCausedBy>
            ALWAYS_INLINE void logExceptionRecursively(const std::system_error& e) const {
                return logExceptionRecursivelyImpl<std::system_error, PrintCausedBy>(e);
            }

#ifdef Q_OS_WIN
            template<bool PrintCausedBy>
            ALWAYS_INLINE void logExceptionRecursively(const winrt::hresult_error& e) const {
                return logExceptionRecursivelyImpl<winrt::hresult_error, PrintCausedBy>(e);
            }
#endif

            template<IsException E, bool PrintCausedBy>
            void logExceptionRecursivelyImpl(const E& e) const;

            QtMsgType type;
            QMessageLogContext context;
        };

        extern template void
        QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::exception, true>(const std::exception&) const;
        extern template void
        QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::exception, false>(const std::exception&) const;
        extern template void
        QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::system_error, true>(const std::system_error&) const;
        extern template void
        QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::system_error, false>(const std::system_error&) const;
#ifdef Q_OS_WIN
        extern template void
        QMessageLoggerDelegate::logExceptionRecursivelyImpl<winrt::hresult_error, true>(const winrt::hresult_error&)
            const;
        extern template void
        QMessageLoggerDelegate::logExceptionRecursivelyImpl<winrt::hresult_error, false>(const winrt::hresult_error&)
            const;
#endif

        inline constexpr auto printlnFormatString = "{}\n";
    }

    template<typename T>
    void printlnStdout(const T& value) {
        fmt::print(stdout, impl::printlnFormatString, value);
    }

    template<typename... Args>
        requires(sizeof...(Args) != 0)
    void printlnStdout(FORMAT_STRING fmt, Args&&... args) {
        fmt::print(stdout, impl::printlnFormatString, fmt::format(fmt, std::forward<Args>(args)...));
    }
}

#define QMLD(type) \
    tremotesf::impl::QMessageLoggerDelegate(type, QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC)
#define logDebug(...)                QMLD(QtDebugMsg).log(__VA_ARGS__)
#define logDebugWithException(...)   QMLD(QtDebugMsg).logWithException(__VA_ARGS__)
#define logInfo(...)                 QMLD(QtInfoMsg).log(__VA_ARGS__)
#define logInfoWithException(...)    QMLD(QtInfoMsg).logWithException(__VA_ARGS__)
#define logWarning(...)              QMLD(QtWarningMsg).log(__VA_ARGS__)
#define logWarningWithException(...) QMLD(QtWarningMsg).logWithException(__VA_ARGS__)

#endif // TREMOTESF_LOG_LOG_H
