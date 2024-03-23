// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "darkthemeapplier_windows.h"

#include <algorithm>

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QToolTip>
#include <QStyle>
#include <QWidget>
#include <QWindow>

#include <dwmapi.h>

#include <guiddef.h>
#include <winrt/base.h>

#include "log/log.h"
#include "settings.h"
#include "windowshelpers.h"
#include "systemcolorsprovider.h"

SPECIALIZE_FORMATTER_FOR_Q_ENUM(QEvent::Type)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(Qt::WindowType)

namespace tremotesf {

    namespace {
        namespace DWMWINDOWATTRIBUTE_compat {
            inline constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_1809_UNTIL_2004 = 19;
            inline constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_SINCE_2004 = 20;
            inline constexpr DWORD DWMWA_CAPTION_COLOR = 35;
        }

        class TitleBarBackgroundEventFilter final : public QObject {
            Q_OBJECT

        public:
            explicit TitleBarBackgroundEventFilter(
                SystemColorsProvider* systemColorsProvider, QObject* parent = nullptr
            )
                : QObject{parent}, mSystemColorsProvider{systemColorsProvider} {}

            bool eventFilter(QObject* watched, QEvent* event) override {
                switch (event->type()) {
                case QEvent::WinIdChange: {
                    auto window = qobject_cast<QWindow*>(watched);
                    if (!window) {
                        if (auto widget = qobject_cast<QWidget*>(watched); widget) {
                            window = widget->windowHandle();
                        }
                    }
                    if (window) {
                        switch (window->type()) {
                        case Qt::Window:
                        case Qt::Dialog:
                            applyDarkThemeToTitleBarAndConnectSignal(window);
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                }
                default:
                    break;
                }
                return false;
            }

        private:
            static inline constexpr auto signalAddedProperty = "tremotesf::TitleBarBackgroundEventFilter";

            void applyDarkThemeToTitleBarAndConnectSignal(QWindow* window) {
                applyDarkThemeToTitleBar(window);
                if (!window->property(signalAddedProperty).toBool()) {
                    QObject::connect(
                        mSystemColorsProvider,
                        &SystemColorsProvider::darkThemeEnabledChanged,
                        window,
                        [this, window] { applyDarkThemeToTitleBar(window); }
                    );
                    QObject::connect(Settings::instance(), &Settings::darkThemeModeChanged, window, [this, window] {
                        applyDarkThemeToTitleBar(window);
                    });
                    window->setProperty(signalAddedProperty, true);
                }
            }

            void applyDarkThemeToTitleBar(QWindow* window) {
                if (isRunningOnWindows11OrGreater()) {
                    info().log("Setting DWMWA_CAPTION_COLOR on {}", *window);
                    const auto qcolor = QGuiApplication::palette().color(QPalette::Window);
                    const auto color = RGB(qcolor.red(), qcolor.green(), qcolor.blue());
                    try {
                        checkHResult(
                            DwmSetWindowAttribute(
                                reinterpret_cast<HWND>(window->winId()),
                                DWMWINDOWATTRIBUTE_compat::DWMWA_CAPTION_COLOR,
                                &color,
                                sizeof(color)
                            ),
                            "DwmSetWindowAttribute"
                        );
                    } catch (const winrt::hresult_error& e) {
                        warning().logWithException(e, "Failed to set DWMWA_CAPTION_COLOR on {}", *window);
                    }
                } else {
                    info().log("Setting DWMWA_USE_IMMERSIVE_DARK_MODE on {}", *window);
                    const auto attribute = []() -> DWORD {
                        if (isRunningOnWindows10_2004OrGreater()) {
                            return DWMWINDOWATTRIBUTE_compat::DWMWA_USE_IMMERSIVE_DARK_MODE_SINCE_2004;
                        }
                        return DWMWINDOWATTRIBUTE_compat::DWMWA_USE_IMMERSIVE_DARK_MODE_1809_UNTIL_2004;
                    }();
                    const bool darkTheme = [&] {
                        switch (Settings::instance()->darkThemeMode()) {
                        case Settings::DarkThemeMode::FollowSystem:
                            return mSystemColorsProvider->isDarkThemeEnabled();
                        case Settings::DarkThemeMode::On:
                            return true;
                        case Settings::DarkThemeMode::Off:
                            return false;
                        }
                        throw std::logic_error("Unknown DarkThemeMode value");
                    }();
                    const auto useImmersiveDarkMode = static_cast<BOOL>(darkTheme);
                    try {
                        checkHResult(
                            DwmSetWindowAttribute(
                                reinterpret_cast<HWND>(window->winId()),
                                attribute,
                                &useImmersiveDarkMode,
                                sizeof(useImmersiveDarkMode)
                            ),
                            "DwmSetWindowAttribute"
                        );
                    } catch (const winrt::hresult_error& e) {
                        warning().logWithException(e, "Failed to set DWMWA_USE_IMMERSIVE_DARK_MODE on {}", *window);
                    }
                }
            }

            SystemColorsProvider* mSystemColorsProvider{};
        };

        QColor blendAtop(QColor source, QColor background) {
            const int alpha = source.alpha();
            if (alpha == 255) return source;
            const auto blend = [&](int s, int b) { return (s * alpha / 255 + b * (255 - alpha) / 255); };
            return {
                blend(source.red(), background.red()),
                blend(source.green(), background.green()),
                blend(source.blue(), background.blue())
            };
        }

        double getRelativeLuminance(QColor color) {
            const auto trans = [](auto compF) {
                const auto comp = static_cast<double>(compF);
                if (comp <= 0.03928) {
                    return comp / 12.92;
                }
                return std::pow(((comp + 0.055) / 1.055), 2.4);
            };
            return 0.2126 * trans(color.redF()) + 0.7152 * trans(color.greenF()) + 0.0722 * trans(color.blueF());
        }

        // From Web Content Accessibility Guidelines 2.2
        double getContrastRatio(QColor lighterColor, QColor darkerColor) {
            return (getRelativeLuminance(lighterColor) + 0.05) / (getRelativeLuminance(darkerColor) + 0.05);
        }

        void applyWindowsPalette(bool darkTheme, SystemColorsProvider::AccentColors accentColors) {
            QPalette palette{QApplication::style()->standardPalette()};

            const auto darkTextColor = QColor(0, 0, 0, 228);
            const auto darkTextColorDisabled = QColor(0, 0, 0, 92);
            const auto lightTextColor = QColor(255, 255, 255, 255);
            const auto lightTextColorDisabled = QColor(255, 255, 255, 93);
            if (darkTheme) {
                palette.setColor(QPalette::Window, QColor(32, 32, 32, 255));
                palette.setColor(QPalette::WindowText, lightTextColor);
                palette.setColor(QPalette::Disabled, QPalette::WindowText, lightTextColorDisabled);
                palette.setColor(QPalette::Base, QColor(45, 45, 45, 255));
                palette.setColor(QPalette::Disabled, QPalette::Base, QColor(41, 41, 41, 255));
                palette.setColor(QPalette::PlaceholderText, QColor(255, 255, 255, 197));
                palette.setColor(QPalette::Button, QColor(45, 45, 45, 255));
                palette.setColor(QPalette::Disabled, QPalette::Button, QColor(41, 41, 41, 255));
            } else {
                palette.setColor(QPalette::Window, QColor(243, 243, 243, 255));
                palette.setColor(QPalette::WindowText, darkTextColor);
                palette.setColor(QPalette::Disabled, QPalette::WindowText, darkTextColorDisabled);
                palette.setColor(QPalette::Base, QColor(251, 251, 251, 255));
                palette.setColor(QPalette::Disabled, QPalette::Base, QColor(244, 244, 244, 255));
                palette.setColor(QPalette::PlaceholderText, QColor(0, 0, 0, 158));
                palette.setColor(QPalette::Button, QColor(251, 251, 251, 255));
                palette.setColor(QPalette::Disabled, QPalette::Button, QColor(244, 244, 244, 255));
            }

            palette.setColor(QPalette::AlternateBase, palette.color(QPalette::Base));
            palette.setColor(
                QPalette::Disabled,
                QPalette::AlternateBase,
                palette.color(QPalette::Disabled, QPalette::Base)
            );
            palette.setColor(QPalette::Text, palette.color(QPalette::WindowText));
            palette
                .setColor(QPalette::Disabled, QPalette::Text, palette.color(QPalette::Disabled, QPalette::WindowText));
            palette.setColor(QPalette::ButtonText, palette.color(QPalette::WindowText));
            palette.setColor(
                QPalette::Disabled,
                QPalette::ButtonText,
                palette.color(QPalette::Disabled, QPalette::WindowText)
            );
            palette.setColor(QPalette::BrightText, palette.color(QPalette::WindowText));
            palette.setColor(
                QPalette::Disabled,
                QPalette::BrightText,
                palette.color(QPalette::Disabled, QPalette::WindowText)
            );
            palette.setColor(
                QPalette::Disabled,
                QPalette::PlaceholderText,
                palette.color(QPalette::Disabled, QPalette::WindowText)
            );

            const auto buttonColor = palette.color(QPalette::Button);
            const auto buttonColorDisabled = palette.color(QPalette::Disabled, QPalette::Button);
            palette.setColor(QPalette::Light, buttonColor.lighter(150));
            palette.setColor(QPalette::Disabled, QPalette::Light, buttonColorDisabled);
            palette.setColor(QPalette::Midlight, buttonColor.lighter(125));
            palette.setColor(QPalette::Disabled, QPalette::Midlight, buttonColorDisabled);
            palette.setColor(QPalette::Dark, buttonColor.darker(200));
            palette.setColor(QPalette::Disabled, QPalette::Dark, buttonColorDisabled);
            palette.setColor(QPalette::Mid, buttonColor.darker(150));
            palette.setColor(QPalette::Disabled, QPalette::Mid, buttonColorDisabled);

            if (accentColors.isValid()) {
                const auto ratio =
                    getContrastRatio(blendAtop(lightTextColor, accentColors.accentColor), accentColors.accentColor);
                if (std::ceil(ratio * 10.0) >= 45.0) {
                    palette.setColor(QPalette::Highlight, accentColors.accentColor);
                    palette.setColor(QPalette::Inactive, QPalette::Highlight, accentColors.accentColorLight1);
                } else {
                    palette.setColor(QPalette::Highlight, accentColors.accentColorDark1);
                    palette.setColor(QPalette::Inactive, QPalette::Highlight, accentColors.accentColor);
                }
                palette.setColor(QPalette::HighlightedText, lightTextColor);
                palette.setColor(QPalette::Disabled, QPalette::HighlightedText, lightTextColorDisabled);
            }
            const auto highlightColor = palette.color(QPalette::Active, QPalette::Highlight);
            QColor link{};
            if (darkTheme) {
                link = highlightColor.lighter(150);
            } else {
                link = highlightColor.darker(150);
            }
            palette.setColor(QPalette::Link, link);
            palette.setColor(QPalette::LinkVisited, link.darker());

            info().log("applyDarkThemeToPalette: setting application palette");
            QGuiApplication::setPalette(palette);

            QPalette toolTipPalette{QToolTip::palette()};
            if (darkTheme) {
                toolTipPalette.setColor(QPalette::ToolTipBase, QColor(43, 43, 43, 255));
                toolTipPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255, 255));
            } else {
                toolTipPalette.setColor(QPalette::ToolTipBase, QColor(242, 242, 242, 255));
                toolTipPalette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 228));
            }
            QToolTip::setPalette(toolTipPalette);
        }
    }

    void applyDarkThemeToPalette(SystemColorsProvider* systemColorsProvider) {
        const auto settings = Settings::instance();
        const auto apply = [=] {
            const bool darkTheme = [&] {
                switch (settings->darkThemeMode()) {
                case Settings::DarkThemeMode::FollowSystem:
                    return systemColorsProvider->isDarkThemeEnabled();
                case Settings::DarkThemeMode::On:
                    return true;
                case Settings::DarkThemeMode::Off:
                    return false;
                }
                throw std::logic_error("Unknown DarkThemeMode value");
            }();
            const auto accentColors = settings->useSystemAccentColor() ? systemColorsProvider->accentColors()
                                                                       : SystemColorsProvider::AccentColors{};
            applyWindowsPalette(darkTheme, accentColors);
        };
        apply();
        QObject::connect(
            systemColorsProvider,
            &SystemColorsProvider::darkThemeEnabledChanged,
            QGuiApplication::instance(),
            apply
        );
        QObject::connect(
            systemColorsProvider,
            &SystemColorsProvider::accentColorsChanged,
            QGuiApplication::instance(),
            apply
        );
        QObject::connect(settings, &Settings::darkThemeModeChanged, QGuiApplication::instance(), apply);
        QObject::connect(settings, &Settings::useSystemAccentColorChanged, QGuiApplication::instance(), apply);

        QGuiApplication::instance()->installEventFilter(
            new TitleBarBackgroundEventFilter(systemColorsProvider, QGuiApplication::instance())
        );
    }

}

#include "darkthemeapplier_windows.moc"
