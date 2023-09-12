// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTest>
#include <QVariant>

#ifdef Q_OS_WIN
#    include <guiddef.h>
#    include <winrt/base.h>
#endif

#include <fmt/format.h>
#include <fmt/compile.h>

#include "rpc/torrent.h"
#include "log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QStringList)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariant)

using namespace tremotesf;

#ifdef Q_OS_WIN
static constexpr auto E_ACCESSDENIED = static_cast<int32_t>(0x80070005);
#endif

class PrintlnTest final : public QObject {
    Q_OBJECT

private slots:
    void stdoutStringLiteral() {
        printlnStdout("foo");
        printlnStdout("{}", "foo");
        printlnStdout(FMT_STRING("{}"), "foo");
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), "foo");
#endif
    }

    void stdoutStdString() {
        const std::string str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutStdStringView() {
        const std::string_view str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutQString() {
        const QString str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutQStringView() {
        const QString _str = "foo";
        const QStringView str = _str;
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

    void stdoutQLatin1String() {
        const auto str = "foo"_l1;
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), str);
#endif
    }

#if QT_VERSION_MAJOR >= 6
    void stdoutQUtf8StringView() {
        const QUtf8StringView str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#    if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), str);
#    endif
    }

    void stdoutQAnyStringView() {
        const QAnyStringView str = "foo";
        printlnStdout(str);
        printlnStdout("{}", str);
        printlnStdout(FMT_STRING("{}"), str);
#    if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), str);
#    endif
    }
#endif

    void stdoutQVariant() {
        const QVariant value = "foo";
        printlnStdout(value);
        printlnStdout("{}", value);
        printlnStdout(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), value);
#endif
    }

    void stdoutQStringList() {
        const QStringList list{"foo"};
        printlnStdout(list);
        printlnStdout("{}", list);
        printlnStdout(FMT_STRING("{}"), list);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), list);
#endif
    }

    void stdoutTorrent() {
        const Torrent value{};
        printlnStdout(value);
        printlnStdout("{}", value);
        printlnStdout(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), value);
#endif
    }

    void stdoutThis() {
        printlnStdout(*this);
        printlnStdout("{}", *this);
        printlnStdout(FMT_STRING("{}"), *this);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), *this);
#endif
    }

    void stdoutQObject() {
        QObject value{};
        printlnStdout(value);
        printlnStdout("{}", value);
        printlnStdout(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        printlnStdout(fmt::runtime("{}"), value);
#endif
    }

    void infoStringLiteral() {
        logInfo("foo");
        logInfo("{}", "foo");
        logInfo(FMT_STRING("{}"), "foo");
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), "foo");
#endif
    }

    void infoStdString() {
        const std::string str = "foo";
        logInfo(str);
        logInfo("{}", str);
        logInfo(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoStdStringView() {
        const std::string_view str = "foo";
        logInfo(str);
        logInfo("{}", str);
        logInfo(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoQString() {
        const QString str = "foo";
        logInfo(str);
        logInfo("{}", str);
        logInfo(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoQStringView() {
        const QString _str = "foo";
        const QStringView str = _str;
        logInfo(str);
        logInfo("{}", str);
        logInfo(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), str);
#endif
    }

    void infoQLatin1String() {
        const auto str = "foo"_l1;
        logInfo(str);
        logInfo("{}", str);
        logInfo(FMT_STRING("{}"), str);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), str);
#endif
    }

#if QT_VERSION_MAJOR >= 6
    void infoQUtf8StringView() {
        const QUtf8StringView str = "foo";
        logInfo(str);
        logInfo("{}", str);
        logInfo(FMT_STRING("{}"), str);
#    if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), str);
#    endif
    }

    void infoQAnyStringView() {
        const QAnyStringView str = "foo";
        logInfo(str);
        logInfo("{}", str);
        logInfo(FMT_STRING("{}"), str);
#    if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), str);
#    endif
    }
#endif

    void infoQVariant() {
        const QVariant value = "foo";
        logInfo(value);
        logInfo("{}", value);
        logInfo(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), value);
#endif
    }

    void infoQStringList() {
        const QStringList list{"foo"};
        logInfo(list);
        logInfo("{}", list);
        logInfo(FMT_STRING("{}"), list);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), list);
#endif
    }

    void infoTorrent() {
        const Torrent value{};
        logInfo(value);
        logInfo("{}", value);
        logInfo(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), value);
#endif
    }

    void infoThis() {
        logInfo(*this);
        logInfo("{}", *this);
        logInfo(FMT_STRING("{}"), *this);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), *this);
#endif
    }

    void infoQObject() {
        QObject value{};
        logInfo(value);
        logInfo("{}", value);
        logInfo(FMT_STRING("{}"), value);
#if FMT_VERSION >= 80000
        logInfo(fmt::runtime("{}"), value);
#endif
    }

    void warningStdException() {
        const std::runtime_error e("nope");
        logWarning(e);
    }

    void warningWithStdException() {
        const std::runtime_error e("nope");
        logWarningWithException(e, "oh no");
    }

    void warningNested() {
        try {
            try {
                throw std::runtime_error("nope");
            } catch (const std::runtime_error&) {
                std::throw_with_nested(std::runtime_error("higher-level nope"));
            }
        } catch (const std::runtime_error& e) {
            logWarning(e);
        }
    }

    void warningWithNested() {
        try {
            try {
                throw std::runtime_error("nope");
            } catch (const std::runtime_error&) {
                std::throw_with_nested(std::runtime_error("higher-level nope"));
            }
        } catch (const std::runtime_error& e) {
            logWarningWithException(e, "oh no");
        }
    }

#ifdef Q_OS_WIN
    void warningHresultError() {
        winrt::hresult_error e(E_ACCESSDENIED);
        logWarning(e);
    }

    void warningWithHresultError() {
        winrt::hresult_error e(E_ACCESSDENIED);
        logWarningWithException(e, "oh no");
    }

    void warningHresultErrorNested() {
        try {
            try {
                throw winrt::hresult_error(E_ACCESSDENIED);
            } catch (const winrt::hresult_error&) {
                std::throw_with_nested(std::runtime_error("higher-level nope"));
            }
        } catch (const std::runtime_error& e) {
            logWarning(e);
        }
    }

    void warningWithHresultErrorNested() {
        try {
            try {
                throw winrt::hresult_error(E_ACCESSDENIED);
            } catch (const winrt::hresult_error&) {
                std::throw_with_nested(std::runtime_error("higher-level nope"));
            }
        } catch (const std::runtime_error& e) {
            logWarningWithException(e, "oh no");
        }
    }
#endif
};

QTEST_GUILESS_MAIN(PrintlnTest)

#include "log_test.moc"
