// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QIcon>

#include "iconthemesetup.h"

#include "fileutils.h"
#include "startup/recoloringsvgiconengineplugin.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    void setupIconTheme() {
        QIcon::setThemeSearchPaths({resolveExternalBundledResourcesPath("icons"_L1)});
        QIcon::setThemeName(TREMOTESF_BUNDLED_ICON_THEME ""_L1);
        QApplication::setStyle(new RecoloringSvgIconStyle(qApp));
    }
}
