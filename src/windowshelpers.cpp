// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windowshelpers.h"

#include <system_error>

#include <fmt/format.h>

#include <windows.h>

#include <guiddef.h>
#include <winrt/base.h>

#include "log/log.h"

namespace tremotesf {
    namespace {
        /**
         * @brief std::error_category for Win32 errors (returned by GetLastError())
         * Returns UTF-8 strings
         * We need custom implementation instead of std::system_category()
         * because GCC < 12 doesn't support Windows errors at all,
         * and MSVC and GCC >= 12 use FormatMessageA function which may not return UTF-8
         */
        class Win32Category final : public std::error_category {
        public:
            const char* name() const noexcept override { return "Win32Category"; }

            std::string message(int code) const override {
                const wchar_t* wstr{};
                const auto size = FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr,
                    static_cast<DWORD>(code),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                    reinterpret_cast<wchar_t*>(&wstr),
                    0,
                    nullptr
                );
                if (size == 0) return "Unknown error";
                return QString::fromWCharArray(wstr, static_cast<QString::size_type>(size)).trimmed().toStdString();
            }

            static const Win32Category& instance() {
                static const Win32Category category{};
                return category;
            }
        };
    }

    void checkWin32Bool(int win32BoolResult, std::string_view functionName) {
        if (win32BoolResult != FALSE) return;
        // Don't use winrt::check_bool because it doesn't preserve Win32 error code
        // (it is converted to HRESULT)
        const auto error = GetLastError();
        throw std::system_error(
            static_cast<int>(error),
            Win32Category::instance(),
            fmt::format("{} failed with", functionName)
        );
    }

    void checkHResult(int32_t hresult, std::string_view functionName) {
        if (hresult == S_OK) return;
        winrt::hstring message = winrt::hresult_error(hresult).message();
        throw winrt::hresult_error(
            hresult,
            QString::fromStdString(fmt::format("{} failed with: {}", functionName, std::move(message))).toStdWString()
        );
    }
}
