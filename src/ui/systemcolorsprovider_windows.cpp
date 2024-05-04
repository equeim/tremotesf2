// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemcolorsprovider.h"

#include "winrt_base_include_wrapper.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "log/log.h"
#include "windowshelpers.h"

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::ViewManagement;

namespace tremotesf {

    namespace {
        QColor qColor(Color color) { return {color.R, color.G, color.B, color.A}; }
    }

    namespace {
        class SystemColorsProviderWindows final : public SystemColorsProvider {
            Q_OBJECT

        public:
            explicit SystemColorsProviderWindows(QObject* parent = nullptr) : SystemColorsProvider(parent) {
                info().log("System dark theme enabled = {}", mDarkThemeEnabled);
                info().log("System accent colors = {}", mAccentColors);
                revoker = settings.ColorValuesChanged(winrt::auto_revoke, [this](auto...) {
                    QMetaObject::invokeMethod(this, [this] {
                        if (bool newDarkThemeEnabled = isDarkThemeEnabledImpl();
                            newDarkThemeEnabled != mDarkThemeEnabled) {
                            info().log("System dark theme state changed to {}", newDarkThemeEnabled);
                            mDarkThemeEnabled = newDarkThemeEnabled;
                            emit darkThemeEnabledChanged();
                        }
                        if (auto newAccentColors = accentColorsImpl(); newAccentColors != mAccentColors) {
                            info().log("System accent colors changed to {}", newAccentColors);
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
                    qColor(settings.GetColorValue(UIColorType::AccentLight1)),
                    qColor(settings.GetColorValue(UIColorType::AccentDark1)),
                };
            }

            UISettings settings{};
            UISettings::ColorValuesChanged_revoker revoker{};

            bool mDarkThemeEnabled{isDarkThemeEnabledImpl()};
            AccentColors mAccentColors{accentColorsImpl()};
        };
    }

    SystemColorsProvider* SystemColorsProvider::createInstance(QObject* parent) {
        return new SystemColorsProviderWindows(parent);
    }

}

#include "systemcolorsprovider_windows.moc"
