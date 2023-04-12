// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemcolorsprovider.h"

namespace tremotesf {
    SystemColorsProvider* SystemColorsProvider::createInstance(QObject* parent) {
        return new SystemColorsProvider(parent);
    }

    bool SystemColorsProvider::isDarkThemeFollowSystemSupported() { return false; }

    bool SystemColorsProvider::isAccentColorsSupported() { return false; }
}
