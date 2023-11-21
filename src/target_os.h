// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TARGET_OS_H
#define TREMOTESF_TARGET_OS_H

#include <QtGlobal>

namespace tremotesf {
    enum class TargetOs { UnixFreedesktop, UnixAndroid, UnixMacOS, UnixOther, Windows, Other };

    inline constexpr TargetOs targetOs =
#if defined(Q_OS_UNIX)
#    if defined(TREMOTESF_UNIX_FREEDESKTOP)
        TargetOs::UnixFreedesktop;
#    elif defined(Q_OS_ANDROID)
        TargetOs::UnixAndroid;
#    elif defined(Q_OS_MACOS)
        TargetOs::UnixMacOS;
#    else
        TargetOs::UnixOther;
#    endif
#elif defined(Q_OS_WIN)
        TargetOs::Windows;
#else
        TargetOs::Other;
#endif

    inline constexpr bool isTargetOsWindows{targetOs == TargetOs::Windows};
}

#endif // TREMOTESF_TARGET_OS_H
