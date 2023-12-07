// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "macoshelpers.h"

#include <AppKit/NSApplication.h>
#include <QApplication>
#include <QStyle>
#include <QProxyStyle>
#include "literals.h"

namespace tremotesf {
    void hideNSApp() { [NSApp hide:nullptr]; }

    void unhideNSApp() { [NSApp unhide:nullptr]; }

    bool isNSAppHidden() { return [NSApp isHidden]; }
}
