// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_COMMONDELEGATE_H
#define TREMOTESF_COMMONDELEGATE_H

#include <optional>
#include <QStyledItemDelegate>

namespace tremotesf {
    class CommonDelegate final : public QStyledItemDelegate {
        Q_OBJECT

    public:
        explicit CommonDelegate(
            std::optional<int> progressBarColumn,
            std::optional<int> progressRole,
            std::optional<int> textElideModeRole,
            QObject* parent = nullptr
        )
            : QStyledItemDelegate(parent),
              mProgressBarColumn(progressBarColumn),
              mProgressRole(progressRole),
              mTextElideModeRole(textElideModeRole){};

        explicit CommonDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

        bool helpEvent(
            QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index
        ) override;

    private:
        std::optional<int> mProgressBarColumn{};
        std::optional<int> mProgressRole{};
        std::optional<int> mTextElideModeRole{};
    };
}

#endif // TREMOTESF_COMMONDELEGATE_H
