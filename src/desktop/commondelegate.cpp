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

#include <QApplication>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleOptionProgressBar>

namespace tremotesf
{
    CommonDelegate::CommonDelegate(int progressBarColumn, int progressBarRole, QObject* parent)
        : QStyledItemDelegate(parent),
          mProgressBarColumn(progressBarColumn),
          mProgressBarRole(progressBarRole)
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
            progressBar.progress = index.data(mProgressBarRole).toDouble() * 100;
            if (progressBar.progress == 0) {
                progressBar.progress = 1;
            }
            progressBar.state = option.state;

#ifdef Q_OS_WIN
            // hack to remove progress bar animation
            if (qApp->style()->objectName() == QLatin1String("windowsvista")) {
                QProxyStyle(QLatin1String("windowsvista")).drawControl(QStyle::CE_ProgressBar, &progressBar, painter);
                return;
            }
#endif
            qApp->style()->drawControl(QStyle::CE_ProgressBar, &progressBar, painter);
        }
    }

    // same height for all indexes
    QSize CommonDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        static int height = 0;
        QSize size(QStyledItemDelegate::sizeHint(option, index));
        if (size.height() > height) {
            height = size.height();
        } else {
            size.setHeight(height);
        }
        return size;
    }
}
