// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "formatters.h"

#include <string_view>

// If we don't include it here we will get undefined reference link error for fmt::formatter<fmt::string_view>
#if FMT_VERSION >= 80000
#    include <fmt/format.h>
#endif

#include <QtGlobal>
#include <QByteArray>
#include <QLatin1String>
#include <QStringView>

#if QT_VERSION_MAJOR >= 6
#    include <QAnyStringView>
#endif

#ifdef Q_OS_WIN
#    include <guiddef.h>
#    include <winrt/base.h>
#endif

#include "demangle.h"

namespace tremotesf::impl {
    template<std::integral Integer>
    fmt::format_context::iterator formatQEnumImpl(const QMetaEnum& meta, Integer value, fmt::format_context& ctx) {
        std::string unnamed{};
        const char* key = [&]() -> const char* {
            if (auto named = meta.valueToKey(static_cast<int>(value)); named) {
                return named;
            }
            unnamed = fmt::format("<unnamed value {}>", value);
            return unnamed.c_str();
        }();
        return fmt::format_to(ctx.out(), "{}::{}::{}", meta.scope(), meta.enumName(), key);
    }

    fmt::format_context::iterator formatQEnum(const QMetaEnum& meta, qint64 value, fmt::format_context& ctx) {
        return formatQEnumImpl(meta, value, ctx);
    }

    fmt::format_context::iterator formatQEnum(const QMetaEnum& meta, quint64 value, fmt::format_context& ctx) {
        return formatQEnumImpl(meta, value, ctx);
    }
}

namespace {
    fmt::string_view toFmtStringView(const QByteArray& str) { return {str.data(), static_cast<size_t>(str.size())}; }

    fmt::format_context::iterator
    formatSystemError(std::string_view type, const std::system_error& e, fmt::format_context& ctx) {
        const int code = e.code().value();
        return fmt::format_to(
            ctx.out(),
            "{}: {} (error code {} ({:#x}))",
            type,
            e.what(),
            code,
            static_cast<unsigned int>(code)
        );
    }
}

namespace fmt {
    format_context::iterator formatter<QString>::format(const QString& string, format_context& ctx) FORMAT_CONST {
        return formatter<string_view>::format(toFmtStringView(string.toUtf8()), ctx);
    }

    format_context::iterator formatter<QStringView>::format(const QStringView& string, format_context& ctx)
        FORMAT_CONST {
        return formatter<string_view>::format(toFmtStringView(string.toUtf8()), ctx);
    }

    format_context::iterator formatter<QLatin1String>::format(const QLatin1String& string, format_context& ctx)
        FORMAT_CONST {
        return formatter<string_view>::format(std::string_view(string.data(), static_cast<size_t>(string.size())), ctx);
    }

    format_context::iterator formatter<QByteArray>::format(const QByteArray& array, format_context& ctx) FORMAT_CONST {
        return formatter<string_view>::format(toFmtStringView(array), ctx);
    }

#if QT_VERSION_MAJOR >= 6
    format_context::iterator formatter<QUtf8StringView>::format(const QUtf8StringView& string, format_context& ctx)
        FORMAT_CONST {
        return formatter<string_view>::format(string_view(string.data(), static_cast<size_t>(string.size())), ctx);
    }

    format_context::iterator formatter<QAnyStringView>::format(const QAnyStringView& string, format_context& ctx)
        FORMAT_CONST {
        return formatter<QString>::format(string.toString(), ctx);
    }
#endif

    format_context::iterator formatter<std::exception>::format(const std::exception& e, format_context& ctx)
        FORMAT_CONST {
        const auto type = tremotesf::typeName(e);
        const auto what = e.what();
        if (auto s = dynamic_cast<const std::system_error*>(&e); s) {
            return formatSystemError(type, *s, ctx);
        }
        return fmt::format_to(ctx.out(), "{}: {}", type, what);
    }

    format_context::iterator formatter<std::system_error>::format(const std::system_error& e, format_context& ctx)
        FORMAT_CONST {
        return formatSystemError(tremotesf::typeName(e), e, ctx);
    }

#ifdef Q_OS_WIN
    format_context::iterator formatter<winrt::hstring>::format(const winrt::hstring& str, format_context& ctx)
        FORMAT_CONST {
        return formatter<QString>::format(
            QString::fromWCharArray(str.data(), static_cast<QString::size_type>(str.size())),
            ctx
        );
    }

    format_context::iterator formatter<winrt::hresult_error>::format(const winrt::hresult_error& e, format_context& ctx)
        FORMAT_CONST {
        const auto code = e.code().value;
        return fmt::format_to(
            ctx.out(),
            "{}: {} (error code {} ({:#x}))",
            tremotesf::typeName(e),
            e.message(),
            code,
            static_cast<uint32_t>(code)
        );
    }
#endif // Q_OS_WIN
}
