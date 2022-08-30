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

#ifndef TREMOTESF_TORRENTFILESPROXYMODEL_H
#define TREMOTESF_TORRENTFILESPROXYMODEL_H

#include "tremotesf/ui/itemmodels/baseproxymodel.h"

namespace tremotesf
{
    class BaseTorrentFilesModel;

    class TorrentFilesProxyModel : public BaseProxyModel
    {
        Q_OBJECT

    public:
        explicit TorrentFilesProxyModel(BaseTorrentFilesModel* sourceModel = nullptr, int sortRole = Qt::DisplayRole, QObject* parent = nullptr);

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    };
}

#endif // TREMOTESF_TORRENTFILESPROXYMODEL_H
