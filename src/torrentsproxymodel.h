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

#ifndef TREMOTESF_TORRENTSPROXYMODEL_H
#define TREMOTESF_TORRENTSPROXYMODEL_H

#include "baseproxymodel.h"

namespace libtremotesf
{
    class Torrent;
}

namespace tremotesf
{
    class TorrentsModel;

    class TorrentsProxyModel : public BaseProxyModel
    {
        Q_OBJECT
        Q_PROPERTY(QString searchString READ searchString WRITE setSearchString NOTIFY searchStringChanged)
        Q_PROPERTY(StatusFilter statusFilter READ statusFilter WRITE setStatusFilter NOTIFY statusFilterChanged)
        Q_PROPERTY(QString tracker READ tracker WRITE setTracker NOTIFY trackerChanged)
        Q_PROPERTY(QString downloadDirectory READ downloadDirectory WRITE setDownloadDirectory NOTIFY downloadDirectoryChanged)
    public:
        enum StatusFilter
        {
            All,
            Active,
            Downloading,
            Seeding,
            Paused,
            Checking,
            Errored,
            StatusFilterCount
        };
        Q_ENUM(StatusFilter)

        explicit TorrentsProxyModel(TorrentsModel* sourceModel = nullptr, int sortRole = Qt::DisplayRole, QObject* parent = nullptr);

        QString searchString() const;
        void setSearchString(const QString& string);

        StatusFilter statusFilter() const;
        void setStatusFilter(StatusFilter filter);

        QString tracker() const;
        void setTracker(const QString& tracker);

        QString downloadDirectory() const;
        void setDownloadDirectory(const QString& downloadDirectory);

        static bool statusFilterAcceptsTorrent(const libtremotesf::Torrent* torrent, StatusFilter filter);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex&) const override;

    private:
        QString mSearchString;
        StatusFilter mStatusFilter;
        QString mTracker;
        QString mDownloadDirectory;
    signals:
        void searchStringChanged();
        void statusFilterChanged();
        void trackerChanged();
        void downloadDirectoryChanged();
    };
}

#endif // TREMOTESF_TORRENTSPROXYMODEL_H
