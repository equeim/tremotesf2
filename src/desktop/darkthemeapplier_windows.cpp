#include "darkthemeapplier.h"

#include <algorithm>

#include <QGuiApplication>
#include <QPalette>
#include <QWidget>
#include <QWindow>
#include <dwmapi.h>

#include "libtremotesf/println.h"
#include "qapplication.h"
#include "qstyle.h"
#include "systemcolorsprovider.h"
#include "../utils.h"

#undef min
#undef max

SPECIALIZE_FORMATTER_FOR_Q_ENUM(QEvent::Type)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(Qt::WindowType)

namespace tremotesf {

namespace {
    class TitleBarBackgroundEventFilter : public QObject {
        Q_OBJECT
    public:
        explicit TitleBarBackgroundEventFilter(SystemColorsProvider* systemColorsProvider, QObject* parent = nullptr) : QObject{parent}, mSystemColorsProvider{systemColorsProvider} {}

        bool eventFilter(QObject* watched, QEvent* event) override {
            switch (event->type()) {
            case QEvent::WinIdChange:
            {
                QWindow* window = nullptr;
                window = qobject_cast<QWindow*>(watched);
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

        void applyDarkThemeToTitleBarAndConnectSignal(QWindow* window)
        {
            applyDarkThemeToTitleBar(window);
            if (!window->property(signalAddedProperty).toBool()) {
                QObject::connect(mSystemColorsProvider, &SystemColorsProvider::darkThemeEnabledChanged, window, [=] {
                    applyDarkThemeToTitleBar(window);
                });
                window->setProperty(signalAddedProperty, true);
            }
        }

        void applyDarkThemeToTitleBar(QWindow* window)
        {
            try {
                printlnInfo("Setting DWMWA_USE_IMMERSIVE_DARK_MODE on {}", *window);
                const auto useImmersiveDarkMode = static_cast<BOOL>(mSystemColorsProvider->isDarkThemeEnabled());
                Utils::callCOMFunction([&] {
                   return DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()), DWMWA_USE_IMMERSIVE_DARK_MODE, &useImmersiveDarkMode, sizeof(useImmersiveDarkMode));
                });
            } catch (const std::exception& e) {
                printlnWarning("DwmSetWindowAttribute failed: {}", e.what());
            }

            try {
                printlnInfo("Setting DWMWA_CAPTION_COLOR on {}", *window);
                const auto qcolor = QGuiApplication::palette().color(QPalette::Window);
                const auto color = RGB(qcolor.red(), qcolor.green(), qcolor.blue());
                Utils::callCOMFunction([&] {
                   return DwmSetWindowAttribute(reinterpret_cast<HWND>(window->winId()), DWMWA_CAPTION_COLOR, &color, sizeof(color));
                });
            } catch (const std::exception& e) {
                printlnWarning("DwmSetWindowAttribute failed: {}", e.what());
            }
        }

        SystemColorsProvider* mSystemColorsProvider{};
    };

    bool IsWindows11OrGreater()
    {
        OSVERSIONINFOEXW info{};
        info.dwOSVersionInfoSize = sizeof(info);
        info.dwMajorVersion = 10;
        info.dwMinorVersion = 0;
        info.dwBuildNumber = 22000;
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

    QColor blendAtop(QColor source, QColor background) {
        const int alpha = source.alpha();
        if (alpha == 255) return source;
        const auto blend = [&](int s, int b) {
            return (s * alpha / 255 + b * (255 - alpha) / 255);
        };
        return QColor(blend(source.red(), background.red()), blend(source.green(), background.green()), blend(source.blue(), background.blue()));
    }

    int getColorBrightness(QColor color) {
        return (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    }

    int getColorDifference(QColor first, QColor second) {
        return (std::max(first.red(), second.red()) - std::min(first.red(), second.red())) +
                (std::max(first.green(), second.green()) - std::min(first.green(), second.green())) +
                (std::max(first.blue(), second.blue()) - std::min(first.blue(), second.blue()));
    }

    int getContrastScore(QColor textColor, QColor backgroundColor) {
        const auto blendedTextColor = blendAtop(textColor, backgroundColor);
        const auto brightnessDifference = std::abs(getColorBrightness(blendedTextColor) - getColorBrightness(backgroundColor));
        const auto colorDifference = getColorDifference(blendedTextColor, backgroundColor);
        return (brightnessDifference - 125) + (colorDifference - 500);
    }

    void applyWindowsPalette(bool darkTheme, QColor highlightColor, QColor highlightColorInactive) {
        QPalette palette{QApplication::style()->standardPalette()};

        const auto textColorLight = QColor(0, 0, 0, 228);
        const auto textColorLightDisabled = QColor(0, 0, 0, 92);
        const auto textColorDark = QColor(255, 255, 255, 255);
        const auto textColorDarkDisabled = QColor(255, 255, 255, 93);
        if (darkTheme) {
            palette.setColor(QPalette::Window, QColor(32, 32, 32, 255));
            palette.setColor(QPalette::WindowText, textColorDark);
            palette.setColor(QPalette::Disabled, QPalette::WindowText, textColorDarkDisabled);
            palette.setColor(QPalette::Base, QColor(255, 255, 255, 15));
            palette.setColor(QPalette::Disabled, QPalette::Base, QColor(255, 255, 255, 11));
            palette.setColor(QPalette::ToolTipBase, QColor(43, 43, 43, 255));
            palette.setColor(QPalette::ToolTipText, QColor(255, 255, 255, 255));
            palette.setColor(QPalette::PlaceholderText, QColor(255, 255, 255, 197));
            palette.setColor(QPalette::Button, QColor(45, 45, 45, 255));
            palette.setColor(QPalette::Disabled, QPalette::Button, QColor(41, 41, 41, 255));
        } else {
            palette.setColor(QPalette::Window, QColor(243, 243, 243, 255));
            palette.setColor(QPalette::WindowText, textColorLight);
            palette.setColor(QPalette::Disabled, QPalette::WindowText, textColorLightDisabled);
            palette.setColor(QPalette::Base, QColor(255, 255, 255, 179));
            palette.setColor(QPalette::Disabled, QPalette::Base, QColor(249, 249, 249, 77));
            palette.setColor(QPalette::ToolTipBase, QColor(242, 242, 242, 255));
            palette.setColor(QPalette::ToolTipText, QColor(0, 0, 0, 228));
            palette.setColor(QPalette::PlaceholderText, QColor(0, 0, 0, 158));
            palette.setColor(QPalette::Button, QColor(251, 251, 251, 255));
            palette.setColor(QPalette::Disabled, QPalette::Button, QColor(244, 244, 244, 255));
        }

        palette.setColor(QPalette::AlternateBase, palette.color(QPalette::Base));
        palette.setColor(QPalette::Disabled, QPalette::AlternateBase, palette.color(QPalette::Disabled, QPalette::Base));
        palette.setColor(QPalette::Text, palette.color(QPalette::WindowText));
        palette.setColor(QPalette::Disabled, QPalette::Text, palette.color(QPalette::Disabled, QPalette::WindowText));
        palette.setColor(QPalette::ButtonText, palette.color(QPalette::WindowText));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, palette.color(QPalette::Disabled, QPalette::WindowText));
        palette.setColor(QPalette::BrightText, palette.color(QPalette::WindowText));
        palette.setColor(QPalette::Disabled, QPalette::BrightText, palette.color(QPalette::Disabled, QPalette::WindowText));
        palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, palette.color(QPalette::Disabled, QPalette::WindowText));

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

        if (highlightColor.isValid()) {
            palette.setColor(QPalette::Highlight, highlightColor);
            palette.setColor(QPalette::Inactive, QPalette::Highlight, highlightColorInactive);
        } else {
            highlightColor = palette.color(QPalette::Active, QPalette::Highlight);
        }
        if (getContrastScore(textColorDark, highlightColor) >= getContrastScore(textColorLight, highlightColor)) {
            palette.setColor(QPalette::HighlightedText, textColorDark);
            palette.setColor(QPalette::Disabled, QPalette::HighlightedText, textColorDarkDisabled);
        } else {
            palette.setColor(QPalette::HighlightedText, textColorLight);
            palette.setColor(QPalette::Disabled, QPalette::HighlightedText, textColorLightDisabled);
        }

        QColor link{};
        if (darkTheme) {
            link = highlightColor.lighter(150);
        } else {
            link = highlightColor.darker(150);
        }
        palette.setColor(QPalette::Link, link);
        palette.setColor(QPalette::LinkVisited, link.darker());

        printlnInfo("applyDarkThemeToPalette: setting application palette");
        QGuiApplication::setPalette(palette);
    }
}

void applyDarkThemeToPalette(SystemColorsProvider* systemColorsProvider)
{
    const auto apply = [=] {
        const auto colors = systemColorsProvider->accentColors();
        applyWindowsPalette(systemColorsProvider->isDarkThemeEnabled(), colors.accentColor, colors.accentColorLight1);
    };
    apply();
    QObject::connect(systemColorsProvider, &SystemColorsProvider::darkThemeEnabledChanged, QGuiApplication::instance(), apply);
    QObject::connect(systemColorsProvider, &SystemColorsProvider::accentColorsChanged, QGuiApplication::instance(), apply);

    if (IsWindows11OrGreater()) {
        printlnInfo("applyDarkThemeToPalette: running on Windows 11 or newer, set title bar color");
        QGuiApplication::instance()->installEventFilter(new TitleBarBackgroundEventFilter(systemColorsProvider, QGuiApplication::instance()));
    } else {
        printlnInfo("applyDarkThemeToPalette: running on Windows older than 11, can't set title bar color");
    }
}

}

#include "darkthemeapplier_windows.moc"
