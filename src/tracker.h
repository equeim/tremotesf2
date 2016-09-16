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

#ifndef TREMOTESF_TRACKER_H
#define TREMOTESF_TRACKER_H

#include <QString>
#include <QVariantMap>

namespace tremotesf
{
    class Tracker
    {
    public:
        explicit Tracker(int id, const QVariantMap& trackerMap);

        int id() const;
        const QString& announce() const;
        const QString& site() const;

        QString status() const;
        bool error() const;
        int peers() const;
        int nextUpdate() const;

        void update(const QVariantMap& trackerMap);
    private:
        enum Status
        {
            Inactive,
            Waiting,
            Queued,
            Active
        };

        int mId;
        QString mAnnounce;
        QString mSite;

        Status mStatus;
        bool mError;
        QString mMessage;
        int mPeers;
        int mNextUpdate;
    };
}

#endif // TREMOTESF_TRACKER_H
