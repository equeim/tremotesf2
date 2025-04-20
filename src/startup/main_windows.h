// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTEST_MAIN_WINDOWS_H
#define TREMOTEST_MAIN_WINDOWS_H

namespace tremotesf {
    class WindowsLogger final {
    public:
        WindowsLogger();
        ~WindowsLogger();
    };

    class WinrtApartment final {
    public:
        WinrtApartment();
        ~WinrtApartment();
    };

    void windowsInitApplication();
}

#endif // TREMOTEST_MAIN_WINDOWS_H
