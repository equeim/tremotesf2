// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "formatters.h"

#include <string_view>

// If we don't include it here we will get undefined reference link error for fmt::formatter<fmt::string_view>
#include <fmt/format.h>

#include <QAnyStringView>
#include <QByteArray>
#include <QLatin1String>
#include <QStringView>

#ifdef Q_OS_WIN
#    include <guiddef.h>
#    include <winrt/base.h>
#endif

#include "demangle.h"

namespace {
    template<std::integral Integer>
    fmt::format_context::iterator formatQEnumImpl(QMetaEnum meta, Integer value, fmt::format_context& ctx) {
        const auto named =
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
            meta.valueToKey(static_cast<quint64>(value));
#else
            meta.valueToKey(static_cast<int>(value));
#endif
        std::string unnamed{};
        const auto string = [&] {
            if (named) return std::string_view(named);
            unnamed = fmt::format("<unnamed value {}>", value);
            return std::string_view(unnamed);
        }();
        return fmt::format_to(ctx.out(), "{}::{}::{}", meta.scope(), meta.enumName(), string);
    }
}

namespace tremotesf {
    fmt::format_context::iterator formatQEnum(QMetaEnum meta, std::intmax_t value, fmt::format_context& ctx) {
        return formatQEnumImpl(meta, value, ctx);
    }

    fmt::format_context::iterator formatQEnum(QMetaEnum meta, std::uintmax_t value, fmt::format_context& ctx) {
        return formatQEnumImpl(meta, value, ctx);
    }
}

namespace {
    fmt::string_view toFmtStringView(QByteArrayView str) { return {str.data(), static_cast<size_t>(str.size())}; }

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
    format_context::iterator formatter<QString>::format(const QString& string, format_context& ctx) const {
        return formatter<string_view>::format(toFmtStringView(string.toUtf8()), ctx);
    }

    format_context::iterator formatter<QStringView>::format(QStringView string, format_context& ctx) const {
        return formatter<string_view>::format(toFmtStringView(string.toUtf8()), ctx);
    }

    format_context::iterator formatter<QLatin1String>::format(QLatin1String string, format_context& ctx) const {
        return formatter<string_view>::format(std::string_view(string.data(), static_cast<size_t>(string.size())), ctx);
    }

    format_context::iterator formatter<QByteArray>::format(const QByteArray& array, format_context& ctx) const {
        return formatter<string_view>::format(toFmtStringView(array), ctx);
    }

    format_context::iterator formatter<QByteArrayView>::format(QByteArrayView array, format_context& ctx) const {
        return formatter<string_view>::format(toFmtStringView(array), ctx);
    }

    format_context::iterator formatter<QUtf8StringView>::format(QUtf8StringView string, format_context& ctx) const {
        return formatter<string_view>::format(toFmtStringView(string), ctx);
    }

    format_context::iterator formatter<QAnyStringView>::format(QAnyStringView string, format_context& ctx) const {
        return formatter<QString>::format(string.toString(), ctx);
    }

    format_context::iterator formatter<std::exception>::format(const std::exception& e, format_context& ctx) const {
        const auto type = tremotesf::typeName(e);
        const auto what = e.what();
        if (auto s = dynamic_cast<const std::system_error*>(&e); s) {
            return formatSystemError(type, *s, ctx);
        }
        return fmt::format_to(ctx.out(), "{}: {}", type, what);
    }

    format_context::iterator
    formatter<std::system_error>::format(const std::system_error& e, format_context& ctx) const {
        return formatSystemError(tremotesf::typeName(e), e, ctx);
    }

#ifdef Q_OS_WIN
    format_context::iterator formatter<winrt::hstring>::format(const winrt::hstring& str, format_context& ctx) const {
        return formatter<QString>::format(
            QString::fromWCharArray(str.data(), static_cast<QString::size_type>(str.size())),
            ctx
        );
    }

    format_context::iterator
    formatter<winrt::hresult_error>::format(const winrt::hresult_error& e, format_context& ctx) const {
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
