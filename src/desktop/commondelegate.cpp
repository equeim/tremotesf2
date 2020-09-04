/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "commondelegate.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleOptionProgressBar>
#include <QToolTip>

namespace tremotesf
{
    namespace
    {
        bool isTextElided(const QString& text, const QStyleOptionViewItem& option)
        {
            const QFontMetrics metrics(option.font);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
            const int textWidth = metrics.horizontalAdvance(text);
#else
            const int textWidth = metrics.width(text);
#endif

            const auto style = option.widget ? option.widget->style() : qApp->style();
            QRect textRect(style->subElementRect(QStyle::SE_ItemViewItemText, &option, option.widget));
            const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) + 1;
            textRect.adjust(textMargin, 0, -textMargin, 0);

            return textWidth > textRect.width();
        }
    }

    CommonDelegate::CommonDelegate(int progressBarColumn, int progressBarRole, QObject* parent)
        : BaseDelegate(parent),
          mProgressBarColumn(progressBarColumn),
          mProgressBarRole(progressBarRole),
          mMaxHeight(0)
    {
    }

    void CommonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyledItemDelegate::paint(painter, option, index);
        if (index.column() == mProgressBarColumn) {
            QStyleOptionProgressBar progressBar;

            QRect rect(option.rect);
            rect.setHeight(rect.height() - 2);
            rect.setY(rect.y() + 1);

            progressBar.rect = rect;
            progressBar.minimum = 0;
            progressBar.maximum = 100;
            progressBar.progress = static_cast<int>(index.data(mProgressBarRole).toDouble() * 100);
            if (progressBar.progress == 0) {
                progressBar.progress = 1;
            }
            progressBar.state = option.state;

            const auto style = option.widget ? option.widget->style() : qApp->style();

#ifdef Q_OS_WIN
            // hack to remove progress bar animation
            if (style->objectName() == QLatin1String("windowsvista")) {
                QProxyStyle(QLatin1String("windowsvista")).drawControl(QStyle::CE_ProgressBar, &progressBar, painter);
                return;
            }
#endif
            style->drawControl(QStyle::CE_ProgressBar, &progressBar, painter);
        }
    }

    // same height for all indexes
    QSize CommonDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QSize size(QStyledItemDelegate::sizeHint(option, index));
        if (size.height() > mMaxHeight) {
            mMaxHeight = size.height();
        } else {
            size.setHeight(mMaxHeight);
        }
        return size;
    }

    void BaseDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QStyledItemDelegate::paint(painter, option, index);
    }

    bool BaseDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
    {
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
