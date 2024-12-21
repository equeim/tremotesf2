// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "darkthemeapplier_windows.h"

#include <cmath>

#include <QApplication>
#include <QOperatingSystemVersion>
#include <QPalette>
#include <QStyleHints>
#include <QWidget>
#include <QWindow>

#include <dwmapi.h>

#include <guiddef.h>
#include <winrt/base.h>

#include "log/log.h"
#include "settings.h"
#include "windowshelpers.h"
#include "systemcolorsprovider.h"

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
                    // Using QWindow::winId instead of QWidget::winId directly because
                    // QWidget::winId may create native window prematurely which we don't want
                    // QWidget::windowHandle will return nullptr if native window doesn't exist yet so we check for that
                    QWindow* window{};
                    if (auto widget = qobject_cast<QWidget*>(watched); widget) {
                        window = widget->windowHandle();
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
                if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11) {
                    debug().log("Setting DWMWA_CAPTION_COLOR on {}", *window);
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
                    debug().log("Setting DWMWA_USE_IMMERSIVE_DARK_MODE on {}", *window);
                    const auto attribute = []() -> DWORD {
                        if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10_2004) {
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

        double getContrastRatio(QColor lighterColor, QColor darkerColor) {
            return (getRelativeLuminance(lighterColor) + 0.05) / (getRelativeLuminance(darkerColor) + 0.05);
        }

        // From Web Content Accessibility Guidelines 2.2
        bool isLegibleWithWhiteText(QColor backgroundColor) {
            const auto ratio = getContrastRatio(blendAtop(Qt::white, backgroundColor), backgroundColor);
            return std::ceil(ratio * 10.0) >= 45.0;
        }

        QColor withAlpha(QColor color, int alpha) {
            color.setAlpha(alpha);
            return color;
        }

        inline constexpr SystemColorsProvider::AccentColors defaultAccentColors{
            .accentColor = QColor(48, 140, 198),
            .accentColorLight1 = QColor(58, 168, 238),
            .accentColorDark1 = QColor(40, 117, 165),
            .accentColorDark2 = QColor(33, 97, 137)
        };

        void applyAccentToPalette(Settings* settings, SystemColorsProvider* systemColorsProvider) {
            info().log("Applying accent colors to palette");
            SystemColorsProvider::AccentColors accentColors;
            if (settings->useSystemAccentColor()) {
                accentColors = systemColorsProvider->accentColors();
                if (!accentColors.isValid()) {
                    accentColors = defaultAccentColors;
                }
            } else {
                accentColors = defaultAccentColors;
            }
            info().log("Accent colors are {}", accentColors);

            QPalette palette{};
            if (isLegibleWithWhiteText(accentColors.accentColor)) {
                palette.setColor(QPalette::Active, QPalette::Highlight, accentColors.accentColor);
                palette.setColor(QPalette::Disabled, QPalette::Highlight, withAlpha(accentColors.accentColor, 93));
            } else {
                palette.setColor(QPalette::Active, QPalette::Highlight, accentColors.accentColorDark1);
                palette.setColor(QPalette::Disabled, QPalette::Highlight, withAlpha(accentColors.accentColorDark1, 93));
            }
            palette.setColor(QPalette::Active, QPalette::HighlightedText, Qt::white);
            palette.setColor(QPalette::Disabled, QPalette::HighlightedText, withAlpha(Qt::white, 93));
            palette.setColor(
                QPalette::Inactive,
                QPalette::Highlight,
                withAlpha(palette.color(QPalette::Active, QPalette::Highlight), 153)
            );
            palette.setColor(QPalette::Inactive, QPalette::HighlightedText, Qt::white);

            QGuiApplication::setPalette(palette);
            if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
                QPalette checkBoxPalette{};
                checkBoxPalette.setColor(QPalette::Active, QPalette::Base, accentColors.accentColorDark1);
                checkBoxPalette.setColor(QPalette::Active, QPalette::Button, accentColors.accentColorLight1);
                checkBoxPalette.setColor(QPalette::Inactive, QPalette::Base, accentColors.accentColorDark2);
                QApplication::setPalette(checkBoxPalette, "QCheckBox");
                QApplication::setPalette(checkBoxPalette, "QRadioButton");
            }
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
            const auto colorScheme = darkTheme ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
            info().log("Setting color scheme {}", colorScheme);
            qApp->styleHints()->setColorScheme(colorScheme);
            QMetaObject::invokeMethod(
                qApp,
                [=]() { applyAccentToPalette(settings, systemColorsProvider); },
                Qt::QueuedConnection
            );
        };
        apply();
        QObject::connect(systemColorsProvider, &SystemColorsProvider::darkThemeEnabledChanged, qApp, apply);
        QObject::connect(systemColorsProvider, &SystemColorsProvider::accentColorsChanged, qApp, apply);
        QObject::connect(settings, &Settings::darkThemeModeChanged, qApp, apply);
        QObject::connect(settings, &Settings::useSystemAccentColorChanged, qApp, apply);

        qApp->installEventFilter(new TitleBarBackgroundEventFilter(systemColorsProvider, QGuiApplication::instance()));
    }

}

#include "darkthemeapplier_windows.moc"
