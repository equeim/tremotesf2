// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "basetreeview.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QMenu>
#include <QTreeView>

namespace tremotesf {
    void setCommonTreeViewProperties(QTreeView* view, bool isFlatList) {
        view->setAllColumnsShowFocus(true);
        view->setSortingEnabled(true);
        view->setUniformRowHeights(true);
        if (isFlatList) {
            view->setRootIsDecorated(false);
            view->header()->setFirstSectionMovable(true);
        }

        const auto header = view->header();
        header->setContextMenuPolicy(Qt::CustomContextMenu);

        QObject::connect(header, &QHeaderView::customContextMenuRequested, view, [=](QPoint pos) {
            const auto model = view->model();
            if (!model) {
                return;
            }

            const auto menu = new QMenu(view);
            menu->setAttribute(Qt::WA_DeleteOnClose);

            for (int i = 0, max = model->columnCount(); i < max; ++i) {
                const auto action = menu->addAction(model->headerData(i, Qt::Horizontal).toString());
                action->setCheckable(true);
                action->setChecked(!view->isColumnHidden(i));
            }

            QObject::connect(menu, &QMenu::triggered, view, [=](QAction* action) {
                const auto column = static_cast<int>(menu->actions().indexOf(action));
                if (view->isColumnHidden(column)) {
                    view->showColumn(column);
                } else {
                    view->hideColumn(column);
                }
            });

            menu->popup(header->viewport()->mapToGlobal(pos));
        });
    }

}
