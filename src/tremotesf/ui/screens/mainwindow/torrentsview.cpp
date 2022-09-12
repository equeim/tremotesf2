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

#include "tremotesf/ui/widgets/commondelegate.h"
#include "tremotesf/settings.h"
#include "torrentsmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf
{
    TorrentsView::TorrentsView(TorrentsProxyModel* model, QWidget* parent)
        : BaseTreeView(parent)
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        setItemDelegate(
            new CommonDelegate(
                static_cast<int>(TorrentsModel::Column::ProgressBar),
                static_cast<int>(TorrentsModel::Role::Sort),
                static_cast<int>(TorrentsModel::Role::TextElideMode),
                this
            )
        );
        setModel(model);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setRootIsDecorated(false);

        if (!header()->restoreState(Settings::instance()->torrentsViewHeaderState())) {
            const auto hiddenColumns = {
                TorrentsModel::Column::TotalSize,
                TorrentsModel::Column::Priority,
                TorrentsModel::Column::QueuePosition,
                TorrentsModel::Column::AddedDate,
                TorrentsModel::Column::DownloadSpeedLimit,
                TorrentsModel::Column::UploadSpeedLimit,
                TorrentsModel::Column::TotalDownloaded,
                TorrentsModel::Column::TotalUploaded,
                TorrentsModel::Column::LeftUntilDone,
                TorrentsModel::Column::DownloadDirectory,
                TorrentsModel::Column::CompletedSize,
                TorrentsModel::Column::ActivityDate
            };
            for (auto column : hiddenColumns) {
                hideColumn(static_cast<int>(column));
            }
            sortByColumn(static_cast<int>(TorrentsModel::Column::Name), Qt::AscendingOrder);
        }
    }

    TorrentsView::~TorrentsView()
    {
        Settings::instance()->setTorrentsViewHeaderState(header()->saveState());
    }
}
