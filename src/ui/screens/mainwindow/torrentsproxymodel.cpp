// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include "torrentsproxymodel.h"

#include "rpc/torrent.h"
#include "rpc/tracker.h"
#include "settings.h"
#include "torrentsmodel.h"

namespace tremotesf {
    class SortFilterProxyModelHelper final {
    public:
        template<std::invocable Func>
        static void changeRowsFilter(TorrentsProxyModel* model, Func&& func) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
            model->beginFilterChange();
#endif
            func();
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
            model->endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
            model->invalidateRowsFilter();
#endif
        }
    };

    TorrentsProxyModel::TorrentsProxyModel(TorrentsModel* sourceModel, QObject* parent)
        : BaseProxyModel(
              sourceModel,
              static_cast<int>(TorrentsModel::Role::Sort),
              static_cast<int>(TorrentsModel::Column::Name),
              parent
          ),
          mStatusFilterEnabled(Settings::instance()->get_torrentsStatusFilterEnabled()),
          mStatusFilter(Settings::instance()->get_torrentsStatusFilter()),
          mLabelFilterEnabled(Settings::instance()->get_torrentsLabelFilterEnabled()),
          mLabelFilter(Settings::instance()->get_torrentsLabelFilter()),
          mTrackerFilterEnabled(Settings::instance()->get_torrentsTrackerFilterEnabled()),
          mTrackerFilter(Settings::instance()->get_torrentsTrackerFilter()),
          mDownloadDirectoryFilterEnabled(Settings::instance()->get_torrentsDownloadDirectoryFilterEnabled()),
          mDownloadDirectoryFilter(Settings::instance()->get_torrentsDownloadDirectoryFilter()) {}

    QString TorrentsProxyModel::searchString() const { return mSearchString; }

    void TorrentsProxyModel::setSearchString(const QString& string) {
        if (string != mSearchString) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mSearchString = string; });
        }
    }

    bool TorrentsProxyModel::isStatusFilterEnabled() const { return mStatusFilterEnabled; }

    void TorrentsProxyModel::setStatusFilterEnabled(bool enabled) {
        if (enabled != mStatusFilterEnabled) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mStatusFilterEnabled = enabled; });
            Settings::instance()->set_torrentsStatusFilterEnabled(mStatusFilterEnabled);
        }
    }

    TorrentsProxyModel::StatusFilter TorrentsProxyModel::statusFilter() const { return mStatusFilter; }

    void TorrentsProxyModel::setStatusFilter(TorrentsProxyModel::StatusFilter filter) {
        if (filter != mStatusFilter) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mStatusFilter = filter; });
            emit statusFilterChanged();
            Settings::instance()->set_torrentsStatusFilter(mStatusFilter);
        }
    }

    bool TorrentsProxyModel::isLabelFilterEnabled() const { return mLabelFilterEnabled; }

    void TorrentsProxyModel::setLabelFilterEnabled(bool enabled) {
        if (enabled != mLabelFilterEnabled) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mLabelFilterEnabled = enabled; });
            Settings::instance()->set_torrentsLabelFilterEnabled(mLabelFilterEnabled);
        }
    }

    QString TorrentsProxyModel::labelFilter() const { return mLabelFilter; }

    void TorrentsProxyModel::setLabelFilter(const QString& filter) {
        if (filter != mLabelFilter) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mLabelFilter = filter; });
            emit labelFilterChanged();
            Settings::instance()->set_torrentsLabelFilter(mLabelFilter);
        }
    }

    bool TorrentsProxyModel::isTrackerFilterEnabled() const { return mTrackerFilterEnabled; }

    void TorrentsProxyModel::setTrackerFilterEnabled(bool enabled) {
        if (enabled != mTrackerFilterEnabled) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mTrackerFilterEnabled = enabled; });
            Settings::instance()->set_torrentsTrackerFilterEnabled(mTrackerFilterEnabled);
        }
    }

    QString TorrentsProxyModel::trackerFilter() const { return mTrackerFilter; }

    void TorrentsProxyModel::setTrackerFilter(const QString& filter) {
        if (filter != mTrackerFilter) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mTrackerFilter = filter; });
            emit trackerFilterChanged();
            Settings::instance()->set_torrentsTrackerFilter(mTrackerFilter);
        }
    }

    bool TorrentsProxyModel::isDownloadDirectoryFilterEnabled() const { return mDownloadDirectoryFilterEnabled; }

    void TorrentsProxyModel::setDownloadDirectoryFilterEnabled(bool enabled) {
        if (enabled != mDownloadDirectoryFilterEnabled) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] {
                mDownloadDirectoryFilterEnabled = enabled;
            });
            Settings::instance()->set_torrentsDownloadDirectoryFilterEnabled(mDownloadDirectoryFilterEnabled);
        }
    }

    QString TorrentsProxyModel::downloadDirectoryFilter() const { return mDownloadDirectoryFilter; }

    void TorrentsProxyModel::setDownloadDirectoryFilter(const QString& filter) {
        if (filter != mDownloadDirectoryFilter) {
            SortFilterProxyModelHelper::changeRowsFilter(this, [&, this] { mDownloadDirectoryFilter = filter; });
            emit downloadDirectoryFilterChanged();
            Settings::instance()->set_torrentsDownloadDirectoryFilter(mDownloadDirectoryFilter);
        }
    }

    bool TorrentsProxyModel::statusFilterAcceptsTorrent(const Torrent* torrent, StatusFilter filter) {
        using enum StatusFilter;
        switch (filter) {
        case Active:
            return (
                (torrent->data().status == TorrentData::Status::Downloading && !torrent->data().isDownloadingStalled())
                || (torrent->data().status == TorrentData::Status::Seeding && !torrent->data().isSeedingStalled())
            );
        case Downloading:
            return (
                torrent->data().status == TorrentData::Status::Downloading
                || torrent->data().status == TorrentData::Status::QueuedForDownloading
            );
        case Seeding:
            return (
                torrent->data().status == TorrentData::Status::Seeding
                || torrent->data().status == TorrentData::Status::QueuedForSeeding
            );
        case Paused:
            return torrent->data().status == TorrentData::Status::Paused;
        case Checking:
            return (
                torrent->data().status == TorrentData::Status::Checking
                || torrent->data().status == TorrentData::Status::QueuedForChecking
            );
        case Errored:
            return torrent->data().hasError();
        default:
            return true;
        }
    }

    bool TorrentsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex&) const {
        const Torrent* torrent = static_cast<TorrentsModel*>(sourceModel())->torrentAtRow(sourceRow);

        if (!mSearchString.isEmpty() && !torrent->data().name.contains(mSearchString, Qt::CaseInsensitive)) {
            return false;
        }

        if (mStatusFilterEnabled && !statusFilterAcceptsTorrent(torrent, mStatusFilter)) {
            return false;
        }

        if (mLabelFilterEnabled && !mLabelFilter.isEmpty()) {
            if (!std::ranges::contains(torrent->data().labels, mLabelFilter)) {
                return false;
            }
        }

        if (mTrackerFilterEnabled && !mTrackerFilter.isEmpty()) {
            if (!std::ranges::contains(torrent->data().trackers, mTrackerFilter, &Tracker::site)) {
                return false;
            }
        }

        if (mDownloadDirectoryFilterEnabled && !mDownloadDirectoryFilter.isEmpty()) {
            if (torrent->data().downloadDirectory != mDownloadDirectoryFilter) {
                return false;
            }
        }

        return true;
    }
}
