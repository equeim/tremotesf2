#include "systemcolorsprovider.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <VersionHelpers.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "libtremotesf/println.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QColor)

using namespace winrt::Windows::UI::ViewManagement;

namespace tremotesf {

namespace {
    class SystemColorsProviderWindows final : public SystemColorsProvider {
        Q_OBJECT
    public:
        explicit SystemColorsProviderWindows(QObject* parent = nullptr) : SystemColorsProvider(parent) {
            printlnInfo("System dark theme enabled = {}", mDarkThemeEnabled);
            printlnInfo("System accent color = {}", mAccentColor);
            revoker = settings.ColorValuesChanged(winrt::auto_revoke, [=](auto...) {
                QMetaObject::invokeMethod(this, [=] {
                    if (bool newDarkThemeEnabled = isDarkThemeEnabledImpl(); newDarkThemeEnabled != mDarkThemeEnabled) {
                        printlnInfo("System dark theme state changed to {}", newDarkThemeEnabled);
                        mDarkThemeEnabled = newDarkThemeEnabled;
                        emit darkThemeEnabledChanged();
                    }
                    if (QColor newAccentColor = accentColorImpl(); newAccentColor != mAccentColor) {
                        printlnInfo("System accent color changed to {}", newAccentColor);
                        mAccentColor = newAccentColor;
                        emit accentColorChanged();
                    }
                });
            });
        }

        bool isDarkThemeEnabled() const override { return mDarkThemeEnabled; };
        QColor accentColor() const override { return mAccentColor; };

    private:
        bool isDarkThemeEnabledImpl() {
            // Apparently this is the way to do it according to Microsoft
            const auto foreground = settings.GetColorValue(UIColorType::Foreground);
            return (((5 * foreground.G) + (2 * foreground.R) + foreground.B) > (8 * 128));
        }

        QColor accentColorImpl() {
            const auto accent = settings.GetColorValue(UIColorType::Accent);
            return QColor(accent.R, accent.G, accent.B, accent.A);
        }

        UISettings settings{};
        UISettings::ColorValuesChanged_revoker revoker{};

        bool mDarkThemeEnabled{isDarkThemeEnabledImpl()};
        QColor mAccentColor{accentColorImpl()};
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
