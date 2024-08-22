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
    Q_LOGGING_CATEGORY(tremotesfLoggingCategory, tremotesfLoggingCategoryName, QtInfoMsg)

    void overrideDebugLogs(bool enable) {
        constexpr auto loggingRulesEnvVariable = "QT_LOGGING_RULES";
        QByteArray rules = qgetenv(loggingRulesEnvVariable);
        if (!rules.isEmpty()) {
            rules += ";";
        }
        rules += fmt::format("{}.debug={}", tremotesfLoggingCategoryName, enable).c_str();
        qputenv(loggingRulesEnvVariable, rules);
    }

    void Logger::logWithFormatArgs(fmt::string_view fmt, fmt::format_args args) const {
        logImpl(QString::fromStdString(fmt::vformat(fmt, args)));
    }

    void Logger::logImpl(const QString& string) const {
        // We use internal qt_message_output() function here because there are only two methods
        // to output string to QMessageLogger and they have overheads that are unneccessary
        // when we are doing formatting on our own:
        // 1. QDebug marshalls everything through QTextStream
        // 2. QMessageLogger::<>(const char*, ...) overloads perform QString::vasprintf() formatting
        qt_message_output(type, context, string);
    }

    template<impl::IsException E>
    void Logger::logExceptionRecursively(const E& e) const {
        logImpl(QString::fromStdString(fmt::format(" |- Caused by: {}", e)));
        try {
            std::rethrow_if_nested(e);
        } catch (const std::exception& nested) {
            logExceptionRecursively(nested);
        }
#ifdef Q_OS_WIN
        catch (const winrt::hresult_error& nested) {
            logExceptionRecursively(nested);
        }
#endif
        catch (...) {
            logImpl(QStringLiteral(" |- Caused by: unknown exception"));
        }
    }

    template void Logger::logExceptionRecursively<std::exception>(const std::exception&) const;
    template void Logger::logExceptionRecursively<std::system_error>(const std::system_error&) const;
#ifdef Q_OS_WIN
    template void Logger::logExceptionRecursively<winrt::hresult_error>(const winrt::hresult_error&) const;
#endif
}
