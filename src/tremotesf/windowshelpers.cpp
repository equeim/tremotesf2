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
}
