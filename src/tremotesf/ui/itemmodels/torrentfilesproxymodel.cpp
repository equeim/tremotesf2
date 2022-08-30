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

#include "torrentfilesproxymodel.h"

#include "tremotesf/ui/itemmodels/basetorrentfilesmodel.h"

namespace tremotesf
{
    TorrentFilesProxyModel::TorrentFilesProxyModel(BaseTorrentFilesModel* sourceModel, int sortRole, QObject* parent)
        : BaseProxyModel(sourceModel, sortRole, parent)
    {
    }

    bool TorrentFilesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
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
