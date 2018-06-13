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

#ifndef LIBTREMOTESF_TRACKER_H
#define LIBTREMOTESF_TRACKER_H

#include <QVariantMap>

namespace libtremotesf
{
    class Tracker
    {
    public:
        enum Status
        {
            Inactive,
            Active,
            Queued,
            Updating,
            Error
        };

        explicit Tracker(int id, const QVariantMap& trackerMap);

        int id() const;
        const QString& announce() const;
        const QString& site() const;

        Status status() const;
        QString errorMessage() const;

        int peers() const;
        int nextUpdate() const;

        void update(const QVariantMap& trackerMap);

    private:
        int mId;
        QString mAnnounce;
        QString mSite;

        Status mStatus = Inactive;
        QString mErrorMessage;
        int mPeers = 0;
        int mNextUpdate = 0;
    };
}

#endif // LIBTREMOTESF_TRACKER_H
