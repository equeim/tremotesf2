// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QToolTip>

#include "tooltipwhenelideddelegate.h"

namespace tremotesf {
    namespace {
        bool isTextElided(const QString& text, const QStyleOptionViewItem& option) {
            const QFontMetrics metrics(option.font);
            const int textWidth = metrics.horizontalAdvance(text);
            const auto style = option.widget ? option.widget->style() : qApp->style();
            QRect textRect(style->subElementRect(QStyle::SE_ItemViewItemText, &option, option.widget));
            const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) + 1;
            textRect.adjust(textMargin, 0, -textMargin, 0);

            return textWidth > textRect.width();
        }
    }

    bool TooltipWhenElidedDelegate::helpEvent(
        QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index
    ) {
        if (event->type() != QEvent::ToolTip) {
            return QStyledItemDelegate::helpEvent(event, view, option, index);
        }

        if (!index.isValid()) {
            event->ignore();
            return false;
        }

        const auto tooltip = displayText(index.data(Qt::ToolTipRole), QLocale{});
        if (tooltip.isEmpty()) {
            event->ignore();
            return false;
        }

        if (QToolTip::isVisible() && QToolTip::text() == tooltip) {
            event->accept();
            return true;
        }

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        if (!mAlwaysShowTooltip) {
            // Get real item rect
            const QRect intersected(opt.rect.intersected(view->viewport()->rect()));
            opt.rect.setLeft(intersected.left());
            opt.rect.setRight(intersected.right());

            // Show tooltip only if display text is elided
            if (!isTextElided(displayText(index.data(Qt::DisplayRole), opt.locale), opt)) {
                event->ignore();
                return false;
            }
        }

        QToolTip::showText(event->globalPos(), tooltip, view->viewport(), opt.rect);
        event->accept();
        return true;
    }
}
