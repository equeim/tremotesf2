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

#include "tracker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonObject>
#include <QUrl>

namespace libtremotesf
{
    Tracker::Tracker(int id, const QJsonObject& trackerMap)
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

    QString Tracker::errorMessage() const
    {
        return mErrorMessage;
    }

    int Tracker::peers() const
    {
        return mPeers;
    }

    int Tracker::nextUpdate() const
    {
        return mNextUpdate;
    }

    void Tracker::update(const QJsonObject& trackerMap)
    {
        const QString announce(trackerMap.value(QLatin1String("announce")).toString());
        if (announce != mAnnounce) {
            mAnnounce = announce;
            const QUrl url(mAnnounce);
            mSite = url.host();
            const int topLevelDomainSize = url.topLevelDomain().size();
            if (topLevelDomainSize > 0) {
                mSite.remove(0, mSite.lastIndexOf(QLatin1Char('.'), -1 - topLevelDomainSize) + 1);
            }
        }

        const bool scrapeError = (!trackerMap.value(QLatin1String("lastScrapeSucceeded")).toBool() &&
                                  trackerMap.value(QLatin1String("lastScrapeTime")).toInt() != 0);

        const bool announceError = (!trackerMap.value(QLatin1String("lastAnnounceSucceeded")).toBool() &&
                                    trackerMap.value(QLatin1String("lastAnnounceTime")).toInt() != 0);

        if (scrapeError || announceError) {
            mStatus = Error;
            if (scrapeError) {
                mErrorMessage = trackerMap.value(QLatin1String("lastScrapeResult")).toString();
            } else {
                mErrorMessage = trackerMap.value(QLatin1String("lastAnnounceResult")).toString();
            }
        } else {
            switch (int status = trackerMap.value(QLatin1String("announceState")).toInt()) {
            case Inactive:
            case Active:
            case Queued:
            case Updating:
            case Error:
                mStatus = static_cast<Status>(status);
                break;
            default:
                mStatus = Error;
            }
            mErrorMessage.clear();
        }

        mPeers = trackerMap.value(QLatin1String("lastAnnouncePeerCount")).toInt();

        mNextUpdate = static_cast<long long>(trackerMap.value(QLatin1String("nextAnnounceTime")).toDouble()) - QDateTime::currentMSecsSinceEpoch() / 1000;
        if (mNextUpdate < 0) {
            mNextUpdate = -1;
        }
    }
}
