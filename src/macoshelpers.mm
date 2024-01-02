// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "macoshelpers.h"

#include <stdexcept>
#include <AppKit/NSApplication.h>

namespace tremotesf {
    void hideNSApp() { [NSApp hide:nullptr]; }

    void unhideNSApp() { [NSApp unhide:nullptr]; }

    bool isNSAppHidden() { return [NSApp isHidden]; }

    QString bundleResourcesPath() {
        auto* const bundle = [NSBundle mainBundle];
        if (!bundle) {
            throw std::runtime_error("[NSBundle mainBundle] returned null");
        }
        auto* const resourcePath = [bundle resourcePath];
        return QString::fromNSString(resourcePath);
    }
}
