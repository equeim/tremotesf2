// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentsview.h"

#include <QHeaderView>

#include "ui/widgets/commondelegate.h"
#include "settings.h"
#include "torrentsmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf {
    TorrentsView::TorrentsView(TorrentsProxyModel* model, QWidget* parent) : BaseTreeView(parent) {
        setContextMenuPolicy(Qt::CustomContextMenu);
        setItemDelegate(new CommonDelegate(
            static_cast<int>(TorrentsModel::Column::ProgressBar),
            static_cast<int>(TorrentsModel::Role::Sort),
            static_cast<int>(TorrentsModel::Role::TextElideMode),
            this
        ));
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
                TorrentsModel::Column::ActivityDate};
            for (auto column : hiddenColumns) {
                hideColumn(static_cast<int>(column));
            }
            sortByColumn(static_cast<int>(TorrentsModel::Column::Name), Qt::AscendingOrder);
        }
    }

    void TorrentsView::saveState() { Settings::instance()->setTorrentsViewHeaderState(header()->saveState()); }

}
