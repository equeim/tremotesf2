/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#include "torrentsproxymodel.h"

#include "torrentsmodel.h"
#include "torrent.h"
#include "tracker.h"

namespace tremotesf
{
    TorrentsProxyModel::TorrentsProxyModel(TorrentsModel* sourceModel, int sortRole, QObject* parent)
        : BaseProxyModel(sourceModel, sortRole, parent),
          mStatusFilter(All)
    {
    }

    QString TorrentsProxyModel::searchString() const
    {
        return mSearchString;
    }

    void TorrentsProxyModel::setSearchString(const QString& string)
    {
        if (string != mSearchString) {
            mSearchString = string;
            invalidateFilter();
            emit searchStringChanged();
        }
    }

    TorrentsProxyModel::StatusFilter TorrentsProxyModel::statusFilter() const
    {
        return mStatusFilter;
    }

    void TorrentsProxyModel::setStatusFilter(TorrentsProxyModel::StatusFilter filter)
    {
        if (filter != mStatusFilter) {
            mStatusFilter = filter;
            invalidateFilter();
            emit statusFilterChanged();
        }
    }

    QString TorrentsProxyModel::tracker() const
    {
        return mTracker;
    }

    void TorrentsProxyModel::setTracker(const QString& tracker)
    {
        if (tracker != mTracker) {
            mTracker = tracker;
            invalidateFilter();
            emit trackerChanged();
        }
    }

    bool TorrentsProxyModel::statusFilterAcceptsTorrent(const Torrent* torrent, StatusFilter filter)
    {
        switch (filter) {
        case Active:
            return (torrent->status() == Torrent::Downloading || torrent->status() == Torrent::Seeding);
        case Downloading:
            return (torrent->status() == Torrent::Downloading ||
                    torrent->status() == Torrent::StalledDownloading ||
                    torrent->status() == Torrent::QueuedForDownloading);
        case Seeding:
            return (torrent->status() == Torrent::Seeding ||
                    torrent->status() == Torrent::StalledSeeding ||
                    torrent->status() == Torrent::QueuedForSeeding);
        case Paused:
            return torrent->status() == Torrent::Paused;
        case Checking:
            return (torrent->status() == Torrent::Checking ||
                    torrent->status() == Torrent::QueuedForChecking);
        case Errored:
            return torrent->status() == Torrent::Errored;
        default:
            return true;
        }
    }

    bool TorrentsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex&) const
    {
        bool accepts = true;

        const Torrent* torrent = static_cast<TorrentsModel*>(sourceModel())->torrentAtRow(sourceRow);

        if (!mSearchString.isEmpty() &&
            !torrent->name().contains(mSearchString, Qt::CaseInsensitive)) {

            accepts = false;
        }

        if (!statusFilterAcceptsTorrent(torrent, mStatusFilter)) {
            accepts = false;
        }

        if (!mTracker.isEmpty()) {
            bool found = false;
            for (const std::shared_ptr<Tracker>& tracker : torrent->trackers()) {
                if (tracker->site() == mTracker) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                accepts = false;
            }
        }

        return accepts;
    }
}
