#include "windowshelpers.h"

#include <array>
#include <QScopeGuard>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <VersionHelpers.h>

namespace tremotesf {
    namespace {
        bool isWindowsVersionOrGreater(int major, int minor, int build) {
            OSVERSIONINFOEXW info{};
            info.dwOSVersionInfoSize = sizeof(info);
            info.dwMajorVersion = major;
            info.dwMinorVersion = minor;
            info.dwBuildNumber = build;
            const auto conditionMask = VerSetConditionMask(
                VerSetConditionMask(
                    VerSetConditionMask(
                        0, VER_MAJORVERSION, VER_GREATER_EQUAL
                    ),
                    VER_MINORVERSION, VER_GREATER_EQUAL
                ),
                VER_BUILDNUMBER, VER_GREATER_EQUAL
            );
            return VerifyVersionInfoW(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, conditionMask) != FALSE;
        }

        std::string getWin32ErrorString(DWORD error) {
            LPWSTR buffer{};
            const auto guard = QScopeGuard([&] { LocalFree(buffer); });
            const auto formattedChars = FormatMessageW(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                nullptr,
                error,
                0,
                reinterpret_cast<LPWSTR>(&buffer),
                0,
                nullptr
            );
            if (formattedChars != 0) {
                return fmt::format(
                    "{} (error code {})",
                    QString::fromWCharArray(buffer, formattedChars).trimmed(),
                    error
                );
            }
            return fmt::format("Error code {}", error);
        }
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

    /*void callWin32Function(std::function<std::uint32_t()>&& function) {
        const auto result = static_cast<BOOL>(function());
        if (result == FALSE) {
            throw std::runtime_error(getWin32ErrorString(GetLastError()));
        }
    }

    void callCOMFunction(std::function<std::int32_t()>&& function) {
        const auto result = static_cast<HRESULT>(function());
        winrt::check_hresult(result);
    }*/
}
