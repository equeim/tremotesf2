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

    namespace {
        QStyle* baseStyle(QStyle* style) {
            while (style) {
                if (const auto proxyStyle = qobject_cast<QProxyStyle*>(style); proxyStyle) {
                    style = proxyStyle->baseStyle();
                    if (style == proxyStyle) {
                        break;
                    }
                } else {
                    break;
                }
            }
            return style;
        }
    }

    bool isThisMacOSStyle(QStyle* style) {
        style = baseStyle(style);
        return style && style->objectName().compare("macos"_l1, Qt::CaseInsensitive) == 0;
    }

    bool isUsingMacOSStyle() { return isThisMacOSStyle(qApp->style()); }
}
