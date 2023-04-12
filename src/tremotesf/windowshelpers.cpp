// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "windowshelpers.h"

#include <system_error>

#include <windows.h>
#include <VersionHelpers.h>

#include <winrt/base.h>

#include "libtremotesf/formatters.h"

namespace tremotesf {
    namespace {
        bool isWindowsVersionOrGreater(DWORD major, DWORD minor, DWORD build) {
            OSVERSIONINFOEXW info{};
            info.dwOSVersionInfoSize = sizeof(info);
            info.dwMajorVersion = major;
            info.dwMinorVersion = minor;
            info.dwBuildNumber = build;
            const auto conditionMask = VerSetConditionMask(
                VerSetConditionMask(
                    VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
                    VER_MINORVERSION,
                    VER_GREATER_EQUAL
                ),
                VER_BUILDNUMBER,
                VER_GREATER_EQUAL
            );
            return VerifyVersionInfoW(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, conditionMask) !=
                   FALSE;
        }

        /**
         * @brief std::error_category for Win32 errors (returned by GetLastError())
         * Returns UTF-8 strings
         * We need custom implementation instead of std::system_category()
         * because GCC < 12 doesn't support Windows errors at all,
         * and MSVC and GCC >= 12 use FormatMessageA function which may not return UTF-8
         */
        class Win32Category : public std::error_category {
        public:
            const char* name() const noexcept override { return "Win32Category"; }

            std::string message(int code) const override {
                wchar_t* wstr{};
                const auto size = FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr,
                    static_cast<DWORD>(code),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    reinterpret_cast<wchar_t*>(&wstr),
                    0,
                    nullptr
                );
                if (size == 0) return "Unknown error";
                return QString::fromWCharArray(wstr, static_cast<int>(size)).trimmed().toStdString();
            }

            static const Win32Category& instance() {
                static const Win32Category category{};
                return category;
            }
        };
    }

    bool isRunningOnWindows10OrGreater() {
        static const bool is = IsWindows10OrGreater();
        return is;
    }

    bool isRunningOnWindows10_1607OrGreater() {
        static const bool is = isWindowsVersionOrGreater(10, 0, 14393);
        return is;
    }

    bool isRunningOnWindows10_1809OrGreater() {
        static const bool is = isWindowsVersionOrGreater(10, 0, 17763);
        return is;
    }

    bool isRunningOnWindows10_2004OrGreater() {
        static const bool is = isWindowsVersionOrGreater(10, 0, 19041);
        return is;
    }

    bool isRunningOnWindows11OrGreater() {
        static const bool is = isWindowsVersionOrGreater(10, 0, 22000);
        return is;
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
