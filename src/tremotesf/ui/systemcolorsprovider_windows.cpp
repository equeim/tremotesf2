// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemcolorsprovider.h"

// Clang needs this header for winrt/base.h
#include <guiddef.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "libtremotesf/log.h"
#include "tremotesf/windowshelpers.h"

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::ViewManagement;

namespace tremotesf {

    namespace {
        QColor qColor(Color color) { return QColor(color.R, color.G, color.B, color.A); }
    }

    namespace {
        class SystemColorsProviderWindows final : public SystemColorsProvider {
            Q_OBJECT
        public:
            explicit SystemColorsProviderWindows(QObject* parent = nullptr) : SystemColorsProvider(parent) {
                logInfo("System dark theme enabled = {}", mDarkThemeEnabled);
                logInfo("System accent colors = {}", mAccentColors);
                revoker = settings.ColorValuesChanged(winrt::auto_revoke, [=, this](auto...) {
                    QMetaObject::invokeMethod(this, [=, this] {
                        if (bool newDarkThemeEnabled = isDarkThemeEnabledImpl();
                            newDarkThemeEnabled != mDarkThemeEnabled) {
                            logInfo("System dark theme state changed to {}", newDarkThemeEnabled);
                            mDarkThemeEnabled = newDarkThemeEnabled;
                            emit darkThemeEnabledChanged();
                        }
                        if (auto newAccentColors = accentColorsImpl(); newAccentColors != mAccentColors) {
                            logInfo("System accent colors changed to {}", newAccentColors);
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
                if (!isDarkThemeFollowSystemSupported()) {
                    return false;
                }
                // Apparently this is the way to do it according to Microsoft
                const auto foreground = settings.GetColorValue(UIColorType::Foreground);
                return (((5 * foreground.G) + (2 * foreground.R) + foreground.B) > (8 * 128));
            }

            AccentColors accentColorsImpl() {
                return {
                    qColor(settings.GetColorValue(UIColorType::Accent)),
                    qColor(settings.GetColorValue(UIColorType::AccentLight1)),
                    qColor(settings.GetColorValue(UIColorType::AccentDark1)),
                };
            }

            UISettings settings{};
            UISettings::ColorValuesChanged_revoker revoker{};

            bool mDarkThemeEnabled{isDarkThemeEnabledImpl()};
            AccentColors mAccentColors{accentColorsImpl()};
        };

        bool canObserveSystemColors() { return isRunningOnWindows10OrGreater(); }
    }

    SystemColorsProvider* SystemColorsProvider::createInstance(QObject* parent) {
        if (canObserveSystemColors()) {
            logInfo("SystemColorsProvider: running on Windows 10 or newer, observe system colors");
            return new SystemColorsProviderWindows(parent);
        }
        logInfo("SystemColorsProvider: running on Windows older than 10, can't observe system colors");
        return new SystemColorsProvider(parent);
    }

    bool SystemColorsProvider::isDarkThemeFollowSystemSupported() { return isRunningOnWindows10_1607OrGreater(); }

    bool SystemColorsProvider::isAccentColorsSupported() { return canObserveSystemColors(); }

}

#include "systemcolorsprovider_windows.moc"
