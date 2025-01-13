// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"

#include <fmt/format.h>

#ifdef Q_OS_WIN
#    include <guiddef.h>
#    include <winrt/base.h>
#endif

namespace tremotesf {
    constexpr auto defaultLogLevel =
#ifdef QT_DEBUG
        QtDebugMsg;
#else
        QtInfoMsg;
#endif

    Q_LOGGING_CATEGORY(tremotesfLoggingCategory, tremotesfLoggingCategoryName, defaultLogLevel)

    void overrideDebugLogs(bool enable) {
        constexpr auto loggingRulesEnvVariable = "QT_LOGGING_RULES";
        QByteArray rules = qgetenv(loggingRulesEnvVariable);
        if (!rules.isEmpty()) {
            rules += ";";
        }
        rules += fmt::format("{}.debug={}", tremotesfLoggingCategoryName, enable).c_str();
        qputenv(loggingRulesEnvVariable, rules);
    }

    namespace {
        template<typename T>
        void appendNestedException(std::string& out, const T& e) {
            fmt::format_to(std::back_insert_iterator(out), "\nCaused by: {}", e);
        }

        template<typename T>
        void appendNestedExceptions(std::string& out, const T& e) {
            try {
                std::rethrow_if_nested(e);
            } catch (const std::system_error& nested) {
                appendNestedException(out, nested);
                appendNestedExceptions(out, nested);
            } catch (const std::exception& nested) {
                appendNestedException(out, nested);
                appendNestedExceptions(out, nested);
            }
#ifdef Q_OS_WIN
            catch (const winrt::hresult_error& nested) {
                appendNestedException(out, nested);
                appendNestedExceptions(out, nested);
            }
#endif
            catch (...) {
                fmt::format_to(std::back_insert_iterator(out), "\nCaused by: unknown exception");
            }
        }
    }

    template<impl::IsException E>
    std::string formatExceptionRecursivelyImpl(const E& e) {
        std::string out = fmt::format(impl::singleArgumentFormatString, e);
        appendNestedExceptions(out, e);
        return out;
    }

    std::string formatExceptionRecursively(const std::exception& e) { return formatExceptionRecursivelyImpl(e); }
    std::string formatExceptionRecursively(const std::system_error& e) { return formatExceptionRecursivelyImpl(e); }
#ifdef Q_OS_WIN
    std::string formatExceptionRecursively(const winrt::hresult_error& e) { return formatExceptionRecursivelyImpl(e); }
#endif

    QString Logger::formatToQString(fmt::string_view fmt, fmt::format_args args) {
        return QString::fromStdString(fmt::vformat(fmt, args));
    }

    void Logger::logWithFormatArgs(fmt::string_view fmt, fmt::format_args args) const {
        logImpl(formatToQString(fmt, args));
    }

    void Logger::logImpl(const QString& string) const {
        // We use internal qt_message_output() function here because there are only two methods
        // to output string to QMessageLogger and they have overheads that are unneccessary
        // when we are doing formatting on our own:
        // 1. QDebug marshalls everything through QTextStream
        // 2. QMessageLogger::<>(const char*, ...) overloads perform QString::vasprintf() formatting
        qt_message_output(type, context, string);
    }
}
