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

#include "tracker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QUrl>
#include <QVariantMap>

namespace tremotesf
{
    Tracker::Tracker(int id, const QVariantMap& trackerMap)
        : mId(id)
    {
        update(trackerMap);
    }

    int Tracker::id() const
    {
        return mId;
    }

    const QString& Tracker::announce() const
    {
        return mAnnounce;
    }

    const QString& Tracker::site() const
    {
        return mSite;
    }

    QString Tracker::status() const
    {
        if (mError && !mMessage.isEmpty()) {
            return qApp->translate("tremotesf", "Error: %1").arg(mMessage);
        }

        switch (mStatus) {
        case Inactive:
            return qApp->translate("tremotesf", "Inactive");
        case Waiting:
            return qApp->translate("tremotesf", "Active", "Tracker status");
        case Queued:
            return qApp->translate("tremotesf", "Queued");
        case Active:
            return qApp->translate("tremotesf", "Updating");
        }

        return QString();
    }

    bool Tracker::error() const
    {
        return mError;
    }

    int Tracker::peers() const
    {
        return mPeers;
    }

    int Tracker::nextUpdate() const
    {
        return mNextUpdate;
    }

    void Tracker::update(const QVariantMap& trackerMap)
    {
        mAnnounce = trackerMap.value("announce").toString();

        const QUrl url(mAnnounce);
        mSite = url.host();
        const QString topLevelDomain(url.topLevelDomain());
        if (!topLevelDomain.isEmpty()) {
            mSite = mSite.mid(mSite.lastIndexOf('.', -topLevelDomain.size() - 1) + 1);
        }

        mStatus = static_cast<Status>(trackerMap.value("announceState").toInt());
        mError = !trackerMap.value("lastAnnounceSucceeded").toBool();
        mMessage = trackerMap.value("lastAnnounceResult").toString();
        mPeers = trackerMap.value("lastAnnouncePeerCount").toInt();
        if (mStatus == Waiting) {
            mNextUpdate = QDateTime::currentDateTime().secsTo(QDateTime::fromTime_t(trackerMap.value("nextAnnounceTime").toInt()));
        } else {
            mNextUpdate = -1;
        }
    }
}
