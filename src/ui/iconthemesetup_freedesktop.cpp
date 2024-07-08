// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QIcon>
#include <QPalette>

#include "iconthemesetup.h"
#include "literals.h"
#include "log/log.h"

namespace tremotesf {
    namespace {
        bool isPaletteDark() {
            // Can't use QStyleHints::colorScheme since it can lie
            const QPalette palette = QGuiApplication::palette();
            const int windowBackgroundGray = qGray(palette.window().color().rgb());
            return windowBackgroundGray < 192;
        }

        QLatin1String fallbackTheme() { return isPaletteDark() ? "breeze-dark"_l1 : "breeze"_l1; }
    }

    void setupIconTheme() {
        const auto theme = fallbackTheme();
        info().log("Setting {} as fallback icon theme", theme);
        QIcon::setFallbackThemeName(theme);
    }
}
