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
        Q_PROPERTY(bool statusFilterEnabled READ isStatusFilterEnabled WRITE setStatusFilterEnabled NOTIFY statusFilterEnabledChanged)
        Q_PROPERTY(StatusFilter statusFilter READ statusFilter WRITE setStatusFilter NOTIFY statusFilterChanged)
        Q_PROPERTY(bool trackerFilterEnabled READ isTrackerFilterEnabled WRITE setTrackerFilterEnabled NOTIFY trackerFilterEnabledChanged)
        Q_PROPERTY(QString trackerFilter READ trackerFilter WRITE setTrackerFilter NOTIFY trackerFilterChanged)
        Q_PROPERTY(bool downloadDirectoryFilterEnabled READ isDownloadDirectoryFilterEnabled WRITE setDownloadDirectoryFilterEnabled NOTIFY downloadDirectoryFilterEnabledChanged)
        Q_PROPERTY(QString downloadDirectoryFilter READ downloadDirectoryFilter WRITE setDownloadDirectoryFilter NOTIFY downloadDirectoryFilterChanged)
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
        ~TorrentsProxyModel();

        QString searchString() const;
        void setSearchString(const QString& string);

        bool isStatusFilterEnabled() const;
        void setStatusFilterEnabled(bool enabled);

        StatusFilter statusFilter() const;
        void setStatusFilter(StatusFilter filter);

        bool isTrackerFilterEnabled() const;
        void setTrackerFilterEnabled(bool enabled);

        QString trackerFilter() const;
        void setTrackerFilter(const QString& filter);

        bool isDownloadDirectoryFilterEnabled() const;
        void setDownloadDirectoryFilterEnabled(bool enabled);

        QString downloadDirectoryFilter() const;
        void setDownloadDirectoryFilter(const QString& filter);

        static bool statusFilterAcceptsTorrent(const libtremotesf::Torrent* torrent, StatusFilter filter);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex&) const override;

    private:
        QString mSearchString;

        bool mStatusFilterEnabled;
        StatusFilter mStatusFilter;

        bool mTrackerFilterEnabled;
        QString mTrackerFilter;

        bool mDownloadDirectoryFilterEnabled;
        QString mDownloadDirectoryFilter;
    signals:
        void searchStringChanged();

        void statusFilterEnabledChanged();
        void statusFilterChanged();

        void trackerFilterEnabledChanged();
        void trackerFilterChanged();

        void downloadDirectoryFilterEnabledChanged();
        void downloadDirectoryFilterChanged();
    };
}

#endif // TREMOTESF_TORRENTSPROXYMODEL_H
