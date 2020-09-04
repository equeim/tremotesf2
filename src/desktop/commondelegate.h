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

#ifndef TREMOTESF_COMMONDELEGATE_H
#define TREMOTESF_COMMONDELEGATE_H

#include <QStyledItemDelegate>

namespace tremotesf
{
    class BaseDelegate : public QStyledItemDelegate
    {
    public:
        inline explicit BaseDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool helpEvent(QHelpEvent* event,
                       QAbstractItemView* view,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) override;
    };

    class CommonDelegate : public BaseDelegate
    {
    public:
        explicit CommonDelegate(int progressBarColumn, int progressBarRole, int textElideModeRole, QObject* parent = nullptr);
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        int mProgressBarColumn;
        int mProgressBarRole;
        int mTextElideModeRole;
        mutable int mMaxHeight;
    };
}

#endif // TREMOTESF_COMMONDELEGATE_H
