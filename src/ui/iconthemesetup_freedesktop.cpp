// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QIcon>

#include "iconthemesetup.h"
#include "log/log.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        QLatin1String fallbackTheme() { return isPaletteDark() ? "breeze-dark"_L1 : "breeze"_L1; }
    }

    void setupIconTheme() {
        const auto theme = fallbackTheme();
        info().log("Setting {} as fallback icon theme", theme);
        QIcon::setFallbackThemeName(theme);
    }
}
