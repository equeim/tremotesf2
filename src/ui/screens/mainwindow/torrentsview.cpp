// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentsview.h"

#include <set>
#include <QHeaderView>

#include "ui/widgets/progressbardelegate.h"
#include "downloaddirectorydelegate.h"
#include "settings.h"
#include "torrentsmodel.h"
#include "torrentsproxymodel.h"

namespace tremotesf {
    TorrentsView::TorrentsView(TorrentsProxyModel* model, QWidget* parent) : BaseTreeView(parent) {
        setContextMenuPolicy(Qt::CustomContextMenu);
        setItemDelegate(new TooltipWhenElidedDelegate(this));
        setItemDelegateForColumn(
            static_cast<int>(TorrentsModel::Column::ProgressBar),
            new ProgressBarDelegate(static_cast<int>(TorrentsModel::Role::Sort), this)
        );
        setItemDelegateForColumn(
            static_cast<int>(TorrentsModel::Column::DownloadDirectory),
            new DownloadDirectoryDelegate(this)
        );
        setModel(model);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setRootIsDecorated(false);

        const auto header = this->header();
        header->setFirstSectionMovable(true);
        if (!header->restoreState(Settings::instance()->get_torrentsViewHeaderState())) {
            using C = TorrentsModel::Column;
            const std::set defaultColumns{
                C::Name,
                C::TotalSize,
                C::ProgressBar,
                C::Status,
                C::Seeders,
                C::Leechers,
                C::PeersSendingToUs,
                C::PeersGettingFromUs,
                C::DownloadSpeed,
                C::UploadSpeed,
                C::Eta,
                C::Ratio,
                C::AddedDate,
            };
            for (int i = 0, max = header->count(); i < max; ++i) {
                if (!defaultColumns.contains(static_cast<C>(i))) {
                    header->hideSection(i);
                }
            }
            header->moveSection(
                header->visualIndex(static_cast<int>(C::AddedDate)),
                header->visualIndex(static_cast<int>(C::Status)) + 1
            );
            header->moveSection(
                header->visualIndex(static_cast<int>(C::Eta)),
                header->visualIndex(static_cast<int>(C::AddedDate)) + 1
            );
            sortByColumn(static_cast<int>(TorrentsModel::Column::AddedDate), Qt::DescendingOrder);
        }
    }

    void TorrentsView::saveState() { Settings::instance()->set_torrentsViewHeaderState(header()->saveState()); }

}
