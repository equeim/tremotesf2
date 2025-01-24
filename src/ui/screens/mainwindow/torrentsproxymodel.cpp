// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentsproxymodel.h"

#include "rpc/torrent.h"
#include "rpc/tracker.h"
#include "settings.h"
#include "torrentsmodel.h"

namespace tremotesf {
    TorrentsProxyModel::TorrentsProxyModel(TorrentsModel* sourceModel, QObject* parent)
        : BaseProxyModel(
              sourceModel,
              static_cast<int>(TorrentsModel::Role::Sort),
              static_cast<int>(TorrentsModel::Column::Name),
              parent
          ),
          mStatusFilterEnabled(Settings::instance()->get_torrentsStatusFilterEnabled()),
          mStatusFilter(Settings::instance()->get_torrentsStatusFilter()),
          mTrackerFilterEnabled(Settings::instance()->get_torrentsTrackerFilterEnabled()),
          mTrackerFilter(Settings::instance()->get_torrentsTrackerFilter()),
          mDownloadDirectoryFilterEnabled(Settings::instance()->get_torrentsDownloadDirectoryFilterEnabled()),
          mDownloadDirectoryFilter(Settings::instance()->get_torrentsDownloadDirectoryFilter()) {}

    QString TorrentsProxyModel::searchString() const { return mSearchString; }

    void TorrentsProxyModel::setSearchString(const QString& string) {
        if (string != mSearchString) {
            mSearchString = string;
            invalidateFilter();
        }
    }

    bool TorrentsProxyModel::isStatusFilterEnabled() const { return mStatusFilterEnabled; }

    void TorrentsProxyModel::setStatusFilterEnabled(bool enabled) {
        if (enabled != mStatusFilterEnabled) {
            mStatusFilterEnabled = enabled;
            invalidateFilter();
            Settings::instance()->set_torrentsStatusFilterEnabled(mStatusFilterEnabled);
        }
    }

    TorrentsProxyModel::StatusFilter TorrentsProxyModel::statusFilter() const { return mStatusFilter; }

    void TorrentsProxyModel::setStatusFilter(TorrentsProxyModel::StatusFilter filter) {
        if (filter != mStatusFilter) {
            mStatusFilter = filter;
            invalidateFilter();
            emit statusFilterChanged();
            Settings::instance()->set_torrentsStatusFilter(mStatusFilter);
        }
    }

    bool TorrentsProxyModel::isTrackerFilterEnabled() const { return mTrackerFilterEnabled; }

    void TorrentsProxyModel::setTrackerFilterEnabled(bool enabled) {
        if (enabled != mTrackerFilterEnabled) {
            mTrackerFilterEnabled = enabled;
            invalidateFilter();
            Settings::instance()->set_torrentsTrackerFilterEnabled(mTrackerFilterEnabled);
        }
    }

    QString TorrentsProxyModel::trackerFilter() const { return mTrackerFilter; }

    void TorrentsProxyModel::setTrackerFilter(const QString& filter) {
        if (filter != mTrackerFilter) {
            mTrackerFilter = filter;
            invalidateFilter();
            emit trackerFilterChanged();
            Settings::instance()->set_torrentsTrackerFilter(mTrackerFilter);
        }
    }

    bool TorrentsProxyModel::isDownloadDirectoryFilterEnabled() const { return mDownloadDirectoryFilterEnabled; }

    void TorrentsProxyModel::setDownloadDirectoryFilterEnabled(bool enabled) {
        if (enabled != mDownloadDirectoryFilterEnabled) {
            mDownloadDirectoryFilterEnabled = enabled;
            invalidateFilter();
            Settings::instance()->set_torrentsDownloadDirectoryFilterEnabled(mDownloadDirectoryFilterEnabled);
        }
    }

    QString TorrentsProxyModel::downloadDirectoryFilter() const { return mDownloadDirectoryFilter; }

    void TorrentsProxyModel::setDownloadDirectoryFilter(const QString& filter) {
        if (filter != mDownloadDirectoryFilter) {
            mDownloadDirectoryFilter = filter;
            invalidateFilter();
            emit downloadDirectoryFilterChanged();
            Settings::instance()->set_torrentsDownloadDirectoryFilter(mDownloadDirectoryFilter);
        }
    }

    bool TorrentsProxyModel::statusFilterAcceptsTorrent(const Torrent* torrent, StatusFilter filter) {
        switch (filter) {
        case Active:
            return (
                (torrent->data().status == TorrentData::Status::Downloading && !torrent->data().isDownloadingStalled()
                ) ||
                (torrent->data().status == TorrentData::Status::Seeding && !torrent->data().isSeedingStalled())
            );
        case Downloading:
            return (
                torrent->data().status == TorrentData::Status::Downloading ||
                torrent->data().status == TorrentData::Status::QueuedForDownloading
            );
        case Seeding:
            return (
                torrent->data().status == TorrentData::Status::Seeding ||
                torrent->data().status == TorrentData::Status::QueuedForSeeding
            );
        case Paused:
            return torrent->data().status == TorrentData::Status::Paused;
        case Checking:
            return (
                torrent->data().status == TorrentData::Status::Checking ||
                torrent->data().status == TorrentData::Status::QueuedForChecking
            );
        case Errored:
            return torrent->data().hasError();
        default:
            return true;
        }
    }

    bool TorrentsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex&) const {
        bool accepts = true;

        const Torrent* torrent = static_cast<TorrentsModel*>(sourceModel())->torrentAtRow(sourceRow);

        if (!mSearchString.isEmpty() && !torrent->data().name.contains(mSearchString, Qt::CaseInsensitive)) {
            accepts = false;
        }

        if (mStatusFilterEnabled && !statusFilterAcceptsTorrent(torrent, mStatusFilter)) {
            accepts = false;
        }

        if (mTrackerFilterEnabled && !mTrackerFilter.isEmpty()) {
            bool found = false;
            for (const Tracker& tracker : torrent->data().trackers) {
                if (tracker.site() == mTrackerFilter) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                accepts = false;
            }
        }

        if (mDownloadDirectoryFilterEnabled && !mDownloadDirectoryFilter.isEmpty()) {
            if (torrent->data().downloadDirectory != mDownloadDirectoryFilter) {
                accepts = false;
            }
        }

        return accepts;
    }
}
