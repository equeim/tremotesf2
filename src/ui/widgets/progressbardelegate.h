// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_PROGRESSBARDELEGATE_H
#define TREMOTESF_PROGRESSBARDELEGATE_H

#include <QStyledItemDelegate>

namespace tremotesf {
    class ProgressBarDelegate final : public QStyledItemDelegate {
        Q_OBJECT
    public:
        explicit ProgressBarDelegate(int progressRole, QObject* parent = nullptr)
            : QStyledItemDelegate(parent), mProgressRole(progressRole) {};

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        int mProgressRole;
    };
}

#endif // TREMOTESF_PROGRESSBARDELEGATE_H
