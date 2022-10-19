// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTEST_MAIN_WINDOWS_H
#define TREMOTEST_MAIN_WINDOWS_H

namespace tremotesf {
    void windowsInitPrelude();
    void windowsInitWinrt();
    void windowsInitApplication();
    void windowsDeinitWinrt();
    void windowsDeinitPrelude();
}

#endif // TREMOTEST_MAIN_WINDOWS_H
