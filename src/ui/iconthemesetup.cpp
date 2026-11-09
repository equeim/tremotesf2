// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QPalette>

#include "iconthemesetup.h"

namespace tremotesf {
    bool isPaletteDark() {
        // Can't use QStyleHints::colorScheme since it can lie
        const QPalette palette = QGuiApplication::palette();
        const int windowBackgroundGray = qGray(palette.window().color().rgb());
        return windowBackgroundGray < 192;
    }
}
