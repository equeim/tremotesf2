// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_DARKTHEMEAPPLIER_H
#define TREMOTESF_DARKTHEMEAPPLIER_H

class QWindow;

namespace tremotesf {
    class SystemColorsProvider;
    void applyDarkThemeToPalette(SystemColorsProvider* systemColorsProvider);
}

#endif // TREMOTESF_DARKTHEMEAPPLIER_H
