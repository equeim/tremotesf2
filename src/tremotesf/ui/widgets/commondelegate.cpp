// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commondelegate.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QStyle>
#include <QStyleOptionProgressBar>
#include <QToolTip>

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

    CommonDelegate::CommonDelegate(int progressBarColumn, int progressBarRole, int textElideModeRole, QObject* parent)
        : QStyledItemDelegate(parent),
          mProgressBarColumn(progressBarColumn),
          mProgressBarRole(progressBarRole),
          mTextElideModeRole(textElideModeRole) {}

    void CommonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
        QStyleOptionViewItem opt(option);
        if (mTextElideModeRole != -1) {
            opt.textElideMode = index.data(mTextElideModeRole).value<Qt::TextElideMode>();
        }

        QStyledItemDelegate::paint(painter, opt, index);

        if (mProgressBarColumn == -1 || mProgressBarRole == -1) {
            return;
        }

        if (index.column() == mProgressBarColumn) {
            QStyleOptionProgressBar progressBar;

            progressBar.rect = opt.rect.marginsRemoved(QMargins(0, 1, 0, 1));
            progressBar.minimum = 0;
            progressBar.maximum = 100;
            progressBar.progress = static_cast<int>(index.data(mProgressBarRole).toDouble() * 100);
            if (progressBar.progress == 0) {
                progressBar.progress = 1;
            }
            progressBar.state = opt.state | QStyle::State_Horizontal;
            const auto style = opt.widget ? opt.widget->style() : qApp->style();
            style->drawControl(QStyle::CE_ProgressBar, &progressBar, painter);
        }
    }

    bool CommonDelegate::helpEvent(
        QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index
    ) {
        if (event->type() != QEvent::ToolTip) {
            return QStyledItemDelegate::helpEvent(event, view, option, index);
        }

        if (!index.isValid()) {
            event->ignore();
            return false;
        }

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        const QString tooltip(displayText(index.data(Qt::ToolTipRole), opt.locale));
        if (tooltip.isEmpty()) {
            event->ignore();
            return false;
        }

        if (QToolTip::isVisible() && QToolTip::text() == tooltip) {
            event->accept();
            return true;
        }

        // Get real item rect
        const QRect intersected(opt.rect.intersected(view->viewport()->rect()));
        opt.rect.setLeft(intersected.left());
        opt.rect.setRight(intersected.right());

        // Show tooltip only if display text is elided
        if (isTextElided(displayText(index.data(Qt::DisplayRole), opt.locale), opt)) {
            QToolTip::showText(event->globalPos(), tooltip, view->viewport(), opt.rect);
            event->accept();
            return true;
        }

        event->ignore();
        return false;
    }

}
