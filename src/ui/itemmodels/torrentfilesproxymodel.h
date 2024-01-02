// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTFILESPROXYMODEL_H
#define TREMOTESF_TORRENTFILESPROXYMODEL_H

#include "ui/itemmodels/baseproxymodel.h"

namespace tremotesf {
    class BaseTorrentFilesModel;

    class TorrentFilesProxyModel final : public BaseProxyModel {
        Q_OBJECT

    public:
        explicit TorrentFilesProxyModel(
            BaseTorrentFilesModel* sourceModel, int sortRole, int fallbackColumn, QObject* parent = nullptr
        );

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    };
}

#endif // TREMOTESF_TORRENTFILESPROXYMODEL_H
