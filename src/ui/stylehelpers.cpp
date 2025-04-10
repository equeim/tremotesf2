// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "stylehelpers.h"

#include <QAbstractScrollArea>
#include <QApplication>
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
        const auto name = style->name();
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

    std::optional<KnownStyle> determineStyle() { return determineStyle(QApplication::style()); }

    void overrideBreezeFramelessScrollAreaHeuristic(QAbstractScrollArea* widget, bool drawFrame) {
        widget->setProperty("_breeze_force_frame", drawFrame);
    }

    void makeScrollAreaTransparent(QAbstractScrollArea* widget) {
        widget->setFrameShape(QFrame::NoFrame);
        QPalette palette{};
        palette.setColor(widget->viewport()->backgroundRole(), QColor(Qt::transparent));
        widget->setPalette(palette);
    }

}
