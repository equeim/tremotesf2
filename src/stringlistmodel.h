/*
 * Tremotesf
 * Copyright (C) 2015-2022 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_STRINGLISTMODEL_H
#define TREMOTESF_STRINGLISTMODEL_H

#include <vector>

#include <QAbstractListModel>
#include <QString>

#include "libtremotesf/torrent.h"

namespace tremotesf
{
    class StringListModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
        explicit inline StringListModel(const QString& header = {}, QObject* parent = nullptr) : QAbstractListModel(parent), mHeader(header) {};

        inline QVariant data(const QModelIndex& index, int role) const override { return role == Qt::DisplayRole ? mStringList[static_cast<size_t>(index.row())] : QVariant{}; };
        inline QVariant headerData(int, Qt::Orientation orientation, int role) const override { return orientation == Qt::Horizontal && role == Qt::DisplayRole ? mHeader : QVariant{}; };
        inline int rowCount(const QModelIndex& = {}) const override { return static_cast<int>(mStringList.size()); };

        Q_INVOKABLE void setStringList(const std::vector<QString>& stringList);
    private:
        QString mHeader;
        std::vector<QString> mStringList;

        template<typename, typename, typename, typename> friend class ModelListUpdater;
    };
}

#endif // TREMOTESF_STRINGLISTMODEL_H
