// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTSPROXYMODEL_H
#define TREMOTESF_TORRENTSPROXYMODEL_H

#include "tremotesf/ui/itemmodels/baseproxymodel.h"

namespace libtremotesf {
    class Torrent;
}

namespace tremotesf {
    class TorrentsModel;

    class TorrentsProxyModel final : public BaseProxyModel {
        Q_OBJECT

    public:
        enum StatusFilter { All, Active, Downloading, Seeding, Paused, Checking, Errored, StatusFilterCount };
        Q_ENUM(StatusFilter)

        explicit TorrentsProxyModel(
            TorrentsModel* sourceModel = nullptr, int sortRole = Qt::DisplayRole, QObject* parent = nullptr
        );
        ~TorrentsProxyModel() override;
        Q_DISABLE_COPY_MOVE(TorrentsProxyModel)

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
        //void searchStringChanged();
        void statusFilterChanged();
        void trackerFilterChanged();
        void downloadDirectoryFilterChanged();
    };
}

#endif // TREMOTESF_TORRENTSPROXYMODEL_H
