// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "basetreeview.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QMenu>

namespace tremotesf {
    BaseTreeView::BaseTreeView(QWidget* parent) : QTreeView(parent) {
        setAllColumnsShowFocus(true);
        setSortingEnabled(true);
        setUniformRowHeights(true);

        header()->setContextMenuPolicy(Qt::CustomContextMenu);

        QObject::connect(header(), &QHeaderView::customContextMenuRequested, this, [=, this] {
            if (!model()) {
                return;
            }

            QMenu contextMenu;
            for (int i = 0, max = model()->columnCount(); i < max; ++i) {
                QAction* action = contextMenu.addAction(model()->headerData(i, Qt::Horizontal).toString());
                action->setCheckable(true);
                action->setChecked(!isColumnHidden(i));
            }

            QAction* action = contextMenu.exec(QCursor::pos());
            if (action) {
                const auto column = static_cast<int>(contextMenu.actions().indexOf(action));
                if (isColumnHidden(column)) {
                    showColumn(column);
                } else {
                    hideColumn(column);
                }
            }
        });
    }
}
