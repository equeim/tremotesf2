// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TOOLTIPWHENELIDEDDELEGATE_H
#define TREMOTESF_TOOLTIPWHENELIDEDDELEGATE_H

#include <QStyledItemDelegate>

namespace tremotesf {

    class TooltipWhenElidedDelegate : public QStyledItemDelegate {
        Q_OBJECT
    public:
        explicit TooltipWhenElidedDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        bool helpEvent(
            QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index
        ) override;

    protected:
        bool mAlwaysShowTooltip{false};
    };
}

#endif // TREMOTESF_TOOLTIPWHENELIDEDDELEGATE_H
