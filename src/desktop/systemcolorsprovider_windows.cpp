#include "systemcolorsprovider.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <VersionHelpers.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "libtremotesf/println.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QColor)

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::ViewManagement;

namespace tremotesf {

namespace {
    QColor qColor(Color color) {
        return QColor(color.R, color.G, color.B, color.A);
    }
}

namespace {
    class SystemColorsProviderWindows final : public SystemColorsProvider {
        Q_OBJECT
    public:
        explicit SystemColorsProviderWindows(QObject* parent = nullptr) : SystemColorsProvider(parent) {
            printlnInfo("System dark theme enabled = {}", mDarkThemeEnabled);
            printlnInfo("System accent colors = {}", mAccentColors);
            revoker = settings.ColorValuesChanged(winrt::auto_revoke, [=](auto...) {
                QMetaObject::invokeMethod(this, [=] {
                    if (bool newDarkThemeEnabled = isDarkThemeEnabledImpl(); newDarkThemeEnabled != mDarkThemeEnabled) {
                        printlnInfo("System dark theme state changed to {}", newDarkThemeEnabled);
                        mDarkThemeEnabled = newDarkThemeEnabled;
                        emit darkThemeEnabledChanged();
                    }
                    if (auto newAccentColors = accentColorsImpl(); newAccentColors != mAccentColors) {
                        printlnInfo("System accent colors changed to {}", newAccentColors);
                        mAccentColors = newAccentColors;
                        emit accentColorsChanged();
                    }
                });
            });
        }

        bool isDarkThemeEnabled() const override { return mDarkThemeEnabled; };
        AccentColors accentColors() const override { return mAccentColors; };

    private:
        bool isDarkThemeEnabledImpl() {
            if (!mIsDarkThemeFollowSystemSupported) {
                return false;
            }
            // Apparently this is the way to do it according to Microsoft
            const auto foreground = settings.GetColorValue(UIColorType::Foreground);
            return (((5 * foreground.G) + (2 * foreground.R) + foreground.B) > (8 * 128));
        }

        AccentColors accentColorsImpl() {
            return {
                qColor(settings.GetColorValue(UIColorType::Accent)),
                qColor(settings.GetColorValue(UIColorType::AccentLight1))
            };
        }

        UISettings settings{};
        UISettings::ColorValuesChanged_revoker revoker{};

        bool mDarkThemeEnabled{isDarkThemeEnabledImpl()};
        AccentColors mAccentColors{accentColorsImpl()};

        bool mIsDarkThemeFollowSystemSupported{isDarkThemeFollowSystemSupported()};
    };

    bool canObserveSystemColors() {
        static const bool supported = IsWindows10OrGreater();
        return supported;
    }

    bool IsWindows10AnniversaryUpdateOrGreater()
    {
        OSVERSIONINFOEXW info{};
        info.dwOSVersionInfoSize = sizeof(info);
        info.dwMajorVersion = 10;
        info.dwMinorVersion = 0;
        info.dwBuildNumber = 14393;
        const auto conditionMask = VerSetConditionMask(
            VerSetConditionMask(
                VerSetConditionMask(
                    0, VER_MAJORVERSION, VER_GREATER_EQUAL
                ),
                VER_MINORVERSION, VER_GREATER_EQUAL
            ),
            VER_BUILDNUMBER, VER_GREATER_EQUAL
        );
        return VerifyVersionInfoW(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, conditionMask) == TRUE;
    }
}

SystemColorsProvider* SystemColorsProvider::createInstance(QObject* parent) {
    if (canObserveSystemColors()) {
        printlnInfo("SystemColorsProvider: running on Windows 10 or newer, observe system colors");
        return new SystemColorsProviderWindows(parent);
    }
    printlnInfo("SystemColorsProvider: running on Windows older than 10, can't observe system colors");
    return new SystemColorsProvider(parent);
}

bool SystemColorsProvider::isDarkThemeFollowSystemSupported() {
    return IsWindows10AnniversaryUpdateOrGreater();
}

bool SystemColorsProvider::isAccentColorsSupported() {
    return canObserveSystemColors();
}

}

#include "systemcolorsprovider_windows.moc"
