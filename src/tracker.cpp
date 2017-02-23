/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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
#include <QUrl>

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

    Tracker::Status Tracker::status() const
    {
        return mStatus;
    }

    QString Tracker::statusString() const
    {
        switch (mStatus) {
        case Inactive:
            return qApp->translate("tremotesf", "Inactive");
        case Active:
            return qApp->translate("tremotesf", "Active", "Tracker status");
        case Queued:
            return qApp->translate("tremotesf", "Queued");
        case Updating:
            return qApp->translate("tremotesf", "Updating");
        case Error: {
            if (mErrorMessage.isEmpty()) {
                return qApp->translate("tremotesf", "Error");
            }
            return qApp->translate("tremotesf", "Error: %1").arg(mErrorMessage);
        }
        }

        return QString();
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
        mAnnounce = trackerMap.value(QStringLiteral("announce")).toString();

        const QUrl url(mAnnounce);
        mSite = url.host();
        const QString topLevelDomain(url.topLevelDomain());
        if (!topLevelDomain.isEmpty()) {
            mSite = mSite.mid(mSite.lastIndexOf('.', -topLevelDomain.size() - 1) + 1);
        }

        const bool scrapeError = (!trackerMap.value(QStringLiteral("lastScrapeSucceeded")).toBool() &&
                                  trackerMap.value(QStringLiteral("lastScrapeTime")).toInt() != 0);

        const bool announceError = (!trackerMap.value(QStringLiteral("lastAnnounceSucceeded")).toBool() &&
                                    trackerMap.value(QStringLiteral("lastAnnounceTime")).toInt() != 0);

        if (scrapeError || announceError) {
            mStatus = Error;
            if (scrapeError) {
                mErrorMessage = trackerMap.value(QStringLiteral("lastScrapeResult")).toString();
            } else {
                mErrorMessage = trackerMap.value(QStringLiteral("lastAnnounceResult")).toString();
            }
        } else {
            mStatus = static_cast<Status>(trackerMap.value(QStringLiteral("announceState")).toInt());
            mErrorMessage.clear();
        }

        mPeers = trackerMap.value(QStringLiteral("lastAnnouncePeerCount")).toInt();

        const long long time = trackerMap.value(QStringLiteral("nextAnnounceTime")).toLongLong() * 1000;
        if (time < 0) {
            mNextUpdate = -1;
        } else {
            mNextUpdate = QDateTime::currentDateTime().secsTo(QDateTime::fromMSecsSinceEpoch(time));
        }
    }
}
