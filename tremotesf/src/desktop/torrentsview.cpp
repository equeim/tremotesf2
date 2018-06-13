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

#include "torrentsview.h"

#include <QHeaderView>

#include "../settings.h"
#include "../torrentsmodel.h"
#include "../torrentsproxymodel.h"
#include "commondelegate.h"

namespace tremotesf
{
    TorrentsView::TorrentsView(TorrentsProxyModel* model, QWidget* parent)
        : BaseTreeView(parent)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        setItemDelegate(new CommonDelegate(TorrentsModel::ProgressBarColumn, TorrentsModel::SortRole, this));
        setModel(model);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setRootIsDecorated(false);

        if (!header()->restoreState(Settings::instance()->torrentsViewHeaderState())) {
            hideColumn(TorrentsModel::TotalSizeColumn);
            hideColumn(TorrentsModel::PriorityColumn);
            hideColumn(TorrentsModel::QueuePositionColumn);
            hideColumn(TorrentsModel::AddedDateColumn);
            hideColumn(TorrentsModel::DownloadSpeedLimitColumn);
            hideColumn(TorrentsModel::UploadSpeedLimitColumn);
            hideColumn(TorrentsModel::TotalDownloadedColumn);
            hideColumn(TorrentsModel::TotalUploadedColumn);
            hideColumn(TorrentsModel::LeftUntilDoneColumn);
            hideColumn(TorrentsModel::DownloadDirectoryColumn);
            hideColumn(TorrentsModel::CompletedSizeColumn);
            hideColumn(TorrentsModel::ActivityDateColumn);

            sortByColumn(TorrentsModel::NameColumn, Qt::AscendingOrder);
        }
    }

    TorrentsView::~TorrentsView()
    {
        Settings::instance()->setTorrentsViewHeaderState(header()->saveState());
    }
}
