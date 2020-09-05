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

#ifdef Q_OS_WIN
#include <QProxyStyle>
#endif

namespace tremotesf
{
    class CommonDelegate : public QStyledItemDelegate
    {
    public:
        explicit CommonDelegate(int progressBarColumn, int progressBarRole, int textElideModeRole, QObject* parent = nullptr);
        inline explicit CommonDelegate(QObject* parent = nullptr) : CommonDelegate(-1, -1, -1, parent) {}
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        bool helpEvent(QHelpEvent* event,
                       QAbstractItemView* view,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) override;

    private:
#ifdef Q_OS_WIN
        QProxyStyle mProxyStyle;
#endif
        int mProgressBarColumn;
        int mProgressBarRole;
        int mTextElideModeRole;
        mutable int mMaxHeight;
    };
}

#endif // TREMOTESF_COMMONDELEGATE_H
