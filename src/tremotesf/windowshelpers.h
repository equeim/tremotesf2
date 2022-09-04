#ifndef TREMOTESF_WINDOWSHELPERS_H
#define TREMOTESF_WINDOWSHELPERS_H

#include <functional>

// Clang needs this header for winrt/base.h
#include <guiddef.h>
#include <winrt/base.h>

#include <QString>

#include "libtremotesf/formatters.h"

namespace tremotesf {
    bool isRunningOnWindows10OrGreater();
    bool isRunningOnWindows10_1607OrGreater();
    bool isRunningOnWindows10_1809OrGreater();
    bool isRunningOnWindows10_2004OrGreater();
    bool isRunningOnWindows11OrGreater();
}

template<>
struct fmt::formatter<winrt::hstring> : fmt::formatter<QString> {
    template <typename FormatContext>
    auto format(const winrt::hstring& str, FormatContext& ctx) -> decltype(ctx.out()) {
        return fmt::formatter<QString>::format(
            QString::fromWCharArray(str.data(), static_cast<QString::size_type>(str.size())),
            ctx
        );
    }
};

template<>
struct fmt::formatter<winrt::hresult_error> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const winrt::hresult_error& error, FormatContext& ctx) -> decltype(ctx.out()) {
        return fmt::format_to(
            ctx.out(),
            "{}: {} (error code {})",
            libtremotesf::typeName(error),
            error.message(),
            error.code().value
        );
    }
};

#endif // TREMOTESF_WINDOWSHELPERS_H
