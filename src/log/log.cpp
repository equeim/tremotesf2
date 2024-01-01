// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"

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
}

namespace tremotesf::impl {
    void QMessageLoggerDelegate::log(const QString& string) const {
        // We use internal qt_message_output() function here because there are only two methods
        // to output string to QMessageLogger and they have overheads that are unneccessary
        // when we are doing formatting on our own:
        // 1. QDebug marshalls everything through QTextStream
        // 2. QMessageLogger::<>(const char*, ...) overloads perform QString::vasprintf() formatting
        qt_message_output(type, context, string);
    }

    template<IsException E, bool PrintCausedBy>
    void QMessageLoggerDelegate::logExceptionRecursivelyImpl(const E& e) const {
        if constexpr (PrintCausedBy) {
            log(" |- Caused by: {}", e);
        } else {
            log(singleArgumentFormatString, e);
        }
        try {
            std::rethrow_if_nested(e);
        } catch (const std::exception& nested) {
            logExceptionRecursively<true>(nested);
        }
#ifdef Q_OS_WIN
        catch (const winrt::hresult_error& nested) {
            logExceptionRecursively<true>(nested);
        }
#endif
        catch (...) {
            log(QStringLiteral(" |- Caused by: unknown exception"));
        }
    }

    template void
    QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::exception, true>(const std::exception&) const;
    template void
    QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::exception, false>(const std::exception&) const;
    template void
    QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::system_error, true>(const std::system_error&) const;
    template void
    QMessageLoggerDelegate::logExceptionRecursivelyImpl<std::system_error, false>(const std::system_error&) const;
#ifdef Q_OS_WIN
    template void
    QMessageLoggerDelegate::logExceptionRecursivelyImpl<winrt::hresult_error, true>(const winrt::hresult_error&) const;
    template void
    QMessageLoggerDelegate::logExceptionRecursivelyImpl<winrt::hresult_error, false>(const winrt::hresult_error&) const;
#endif
}
