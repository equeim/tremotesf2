// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "stylehelpers.h"

#include <QProxyStyle>

#include "literals.h"
#include "target_os.h"

namespace tremotesf {
    namespace {
        const QStyle* baseStyle(const QStyle* style) {
            while (style) {
                if (const auto proxyStyle = qobject_cast<const QProxyStyle*>(style); proxyStyle) {
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

    std::optional<KnownStyle> determineStyle(const QStyle* style) {
        style = baseStyle(style);
#if QT_VERSION_MAJOR >= 6
        const auto name = style->name();
#else
        const auto name = style->objectName();
#endif
        if constexpr (targetOs == TargetOs::UnixMacOS) {
            if (name.compare("macos"_l1, Qt::CaseInsensitive) == 0) {
                return KnownStyle::macOS;
            }
        }
        if (name.compare("breeze"_l1, Qt::CaseInsensitive) == 0) {
            return KnownStyle::Breeze;
        }
        return std::nullopt;
    }

}
