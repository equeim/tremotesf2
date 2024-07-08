// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QIcon>

#include "iconthemesetup.h"

#include "fileutils.h"
#include "literals.h"
#include "startup/recoloringsvgiconengineplugin.h"

namespace tremotesf {
    void setupIconTheme() {
        QIcon::setThemeSearchPaths({resolveExternalBundledResourcesPath("icons"_l1)});
        QIcon::setThemeName(TREMOTESF_BUNDLED_ICON_THEME ""_l1);
        QApplication::setStyle(new RecoloringSvgIconStyle(qApp));
    }
}
