// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "listplaceholder.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QVBoxLayout>

namespace tremotesf {

    QLabel* createListPlaceholderLabel(const QString& text) {
        auto* const label = new QLabel(text);
        label->setForegroundRole(QPalette::PlaceholderText);
        label->setTextInteractionFlags(Qt::NoTextInteraction);
        return label;
    }

    void addListPlaceholderLabelToViewportAndManageVisibility(QAbstractItemView* itemView, QLabel* placeholderLabel) {
        auto* const layout = new QVBoxLayout(itemView->viewport());
        layout->addWidget(placeholderLabel);
        layout->setAlignment(placeholderLabel, Qt::AlignCenter);
        auto* const model = itemView->model();
        const auto updateVisibility = [=] { placeholderLabel->setVisible(model->rowCount() == 0); };
        updateVisibility();
        QObject::connect(model, &QAbstractItemModel::rowsInserted, placeholderLabel, updateVisibility);
        QObject::connect(model, &QAbstractItemModel::rowsRemoved, placeholderLabel, updateVisibility);
        QObject::connect(model, &QAbstractItemModel::modelReset, placeholderLabel, updateVisibility);
    }

}
