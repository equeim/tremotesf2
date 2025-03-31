// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_LISTPLACEHOLDER_H
#define TREMOTESF_LISTPLACEHOLDER_H

#include <QString>

class QAbstractItemView;
class QLabel;

namespace tremotesf {
    QLabel* createListPlaceholderLabel(const QString& text = {});
    void addListPlaceholderLabelToViewportAndManageVisibility(QAbstractItemView* itemView, QLabel* placeholderLabel);
}

#endif // TREMOTESF_LISTPLACEHOLDER_H
