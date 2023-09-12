// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfilesproxymodel.h"

#include "ui/itemmodels/basetorrentfilesmodel.h"

namespace tremotesf {
    TorrentFilesProxyModel::TorrentFilesProxyModel(BaseTorrentFilesModel* sourceModel, int sortRole, int fallbackColumn, QObject* parent)
        : BaseProxyModel(sourceModel, sortRole, fallbackColumn, parent) {}

    bool TorrentFilesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
        const bool leftIsDirectory = static_cast<TorrentFilesModelEntry*>(left.internalPointer())->isDirectory();
        const bool rightIsDirectory = static_cast<TorrentFilesModelEntry*>(right.internalPointer())->isDirectory();

        if (leftIsDirectory != rightIsDirectory) {
            if (sortOrder() == Qt::AscendingOrder) {
                return leftIsDirectory;
            }
            return rightIsDirectory;
        }

        return BaseProxyModel::lessThan(left, right);
    }
}
