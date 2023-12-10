// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TARGET_OS_H
#define TREMOTESF_TARGET_OS_H

#include <QtGlobal>

namespace tremotesf {
    enum class TargetOs { UnixFreedesktop, UnixMacOS, Windows };

    inline constexpr TargetOs targetOs =
#if defined(TREMOTESF_UNIX_FREEDESKTOP)
        TargetOs::UnixFreedesktop;
#elif defined(Q_OS_MACOS)
        TargetOs::UnixMacOS;
#elif defined(Q_OS_WIN)
        TargetOs::Windows;
#else
    // We shouldn't even get here since we will fail at CMake configuration step
#    error "Unsupported target platform"
#endif

    inline constexpr bool isTargetOsWindows{targetOs == TargetOs::Windows};
}

#endif // TREMOTESF_TARGET_OS_H
