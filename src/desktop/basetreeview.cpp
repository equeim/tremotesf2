/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include "basetreeview.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QMenu>

namespace tremotesf
{
    BaseTreeView::BaseTreeView(QWidget* parent)
        : QTreeView(parent)
    {
        setAllColumnsShowFocus(true);
        setSortingEnabled(true);
        setUniformRowHeights(true);

        header()->setContextMenuPolicy(Qt::CustomContextMenu);

        QObject::connect(header(), &QHeaderView::customContextMenuRequested, this, [=]() {
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
                const int column = contextMenu.actions().indexOf(action);
                if (isColumnHidden(column)) {
                    showColumn(column);
                } else {
                    hideColumn(column);
                }
            }
        });
    }
}
