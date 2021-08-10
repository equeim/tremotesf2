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

#include <QString>

class QJsonObject;

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

        explicit Tracker(int id, const QJsonObject& trackerMap);

        int id() const;
        const QString& announce() const;
#if QT_VERSION_MAJOR < 6
        const QString& site() const;
#endif
        struct AnnounceHostInfo {
            QString host;
            bool isIpAddress;
        };
        AnnounceHostInfo announceHostInfo() const;

        Status status() const;
        QString errorMessage() const;

        int peers() const;
        long long nextUpdateTime() const;
        int nextUpdateEta() const;

        struct UpdateResult {
            bool changed;
            bool announceUrlChanged;
        };
        UpdateResult update(const QJsonObject& trackerMap);

        inline bool operator==(const Tracker& other) const
        {
            return id() == other.id();
        }

    private:
        QString mAnnounce;
#if QT_VERSION_MAJOR < 6
        QString mSite;
#endif

        QString mErrorMessage;
        Status mStatus = Inactive;

        int mNextUpdateEta = -1;
        long long mNextUpdateTime = 0;

        int mPeers = 0;

        int mId;
    };
}

#endif // LIBTREMOTESF_TRACKER_H
