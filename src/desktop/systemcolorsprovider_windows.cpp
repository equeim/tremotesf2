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
    };
}

std::unique_ptr<SystemColorsProvider> SystemColorsProvider::createInstance() {
    if (IsWindows10OrGreater()) {
        printlnInfo("SystemColorsProvider: running on Windows 10 or newer, observe system colors");
        return std::make_unique<SystemColorsProviderWindows>();
    }
    printlnInfo("SystemColorsProvider: running on Windows older than 10, can't observe system colors");
    return std::make_unique<SystemColorsProvider>();
}

}

#include "systemcolorsprovider_windows.moc"
