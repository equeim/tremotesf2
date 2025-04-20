// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTSPROXYMODEL_H
#define TREMOTESF_TORRENTSPROXYMODEL_H

#include "ui/itemmodels/baseproxymodel.h"

namespace tremotesf {
    class Torrent;
}

namespace tremotesf {
    class TorrentsModel;

    class TorrentsProxyModel final : public BaseProxyModel {
        Q_OBJECT

    public:
        enum StatusFilter { All, Active, Downloading, Seeding, Paused, Checking, Errored, StatusFilterCount };
        Q_ENUM(StatusFilter)

        explicit TorrentsProxyModel(TorrentsModel* sourceModel, QObject* parent = nullptr);
        Q_DISABLE_COPY_MOVE(TorrentsProxyModel)

        QString searchString() const;
        void setSearchString(const QString& string);

        bool isStatusFilterEnabled() const;
        void setStatusFilterEnabled(bool enabled);

        StatusFilter statusFilter() const;
        void setStatusFilter(StatusFilter filter);

        bool isLabelFilterEnabled() const;
        void setLabelFilterEnabled(bool enabled);

        QString labelFilter() const;
        void setLabelFilter(const QString& filter);

        bool isTrackerFilterEnabled() const;
        void setTrackerFilterEnabled(bool enabled);

        QString trackerFilter() const;
        void setTrackerFilter(const QString& filter);

        bool isDownloadDirectoryFilterEnabled() const;
        void setDownloadDirectoryFilterEnabled(bool enabled);

        QString downloadDirectoryFilter() const;
        void setDownloadDirectoryFilter(const QString& filter);

        static bool statusFilterAcceptsTorrent(const Torrent* torrent, StatusFilter filter);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex&) const override;

    private:
        QString mSearchString;

        bool mStatusFilterEnabled;
        StatusFilter mStatusFilter;

        bool mLabelFilterEnabled;
        QString mLabelFilter;

        bool mTrackerFilterEnabled;
        QString mTrackerFilter;

        bool mDownloadDirectoryFilterEnabled;
        QString mDownloadDirectoryFilter;
    signals:
        void statusFilterChanged();
        void labelFilterChanged();
        void trackerFilterChanged();
        void downloadDirectoryFilterChanged();
    };
}

#endif // TREMOTESF_TORRENTSPROXYMODEL_H
