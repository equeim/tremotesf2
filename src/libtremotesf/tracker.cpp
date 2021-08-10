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
#include <QHostAddress>
#include <QUrl>

#include "stdutils.h"

namespace libtremotesf
{
    namespace
    {
        Tracker::Status statusFromInt(int status)
        {
            switch (status) {
            case Tracker::Inactive:
            case Tracker::Active:
            case Tracker::Queued:
            case Tracker::Updating:
            case Tracker::Error:
                return static_cast<Tracker::Status>(status);
            }
            return Tracker::Inactive;
        }
    }

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

#if QT_VERSION_MAJOR < 6
    const QString& Tracker::site() const
    {
        return mSite;
    }
#endif

    Tracker::AnnounceHostInfo Tracker::announceHostInfo() const
    {
        auto host = QUrl(mAnnounce).host();
        bool isIpAddress = !QHostAddress(host).isNull();
        return {std::move(host), isIpAddress};
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

    long long Tracker::nextUpdateTime() const
    {
        return mNextUpdateTime;
    }

    int Tracker::nextUpdateEta() const
    {
        return mNextUpdateEta;
    }

    Tracker::UpdateResult Tracker::update(const QJsonObject& trackerMap)
    {
        bool changed = false;
        bool announceUrlChanged = false;

        QString announce(trackerMap.value(QJsonKeyStringInit("announce")).toString());
        if (announce != mAnnounce) {
            changed = true;
            announceUrlChanged = true;
            mAnnounce = std::move(announce);
#if QT_VERSION_MAJOR < 6
            const QUrl url(mAnnounce);
            mSite = url.host();
            const int topLevelDomainSize = url.topLevelDomain().size();
            if (topLevelDomainSize > 0) {
                mSite.remove(0, mSite.lastIndexOf(QLatin1Char('.'), -1 - topLevelDomainSize) + 1);
            }
#endif
        }

        const bool scrapeError = (!trackerMap.value(QJsonKeyStringInit("lastScrapeSucceeded")).toBool() &&
                                  trackerMap.value(QJsonKeyStringInit("lastScrapeTime")).toInt() != 0);

        const bool announceError = (!trackerMap.value(QJsonKeyStringInit("lastAnnounceSucceeded")).toBool() &&
                                    trackerMap.value(QJsonKeyStringInit("lastAnnounceTime")).toInt() != 0);

        if (scrapeError || announceError) {
            setChanged(mStatus, Error, changed);
            if (scrapeError) {
                setChanged(mErrorMessage, trackerMap.value(QJsonKeyStringInit("lastScrapeResult")).toString(), changed);
            } else {
                setChanged(mErrorMessage, trackerMap.value(QJsonKeyStringInit("lastAnnounceResult")).toString(), changed);
            }
        } else {
            switch (int status = trackerMap.value(QJsonKeyStringInit("announceState")).toInt()) {
            case Inactive:
            case Active:
            case Queued:
            case Updating:
            case Error:
                setChanged(mStatus, statusFromInt(status), changed);
                break;
            default:
                setChanged(mStatus, Error, changed);
                break;
            }
            if (!mErrorMessage.isEmpty()) {
                changed = true;
            }
            mErrorMessage.clear();
        }

        setChanged(mPeers, trackerMap.value(QJsonKeyStringInit("lastAnnouncePeerCount")).toInt(), changed);

        const long long nextUpdateTime = static_cast<long long>(trackerMap.value(QJsonKeyStringInit("nextAnnounceTime")).toDouble());
        if (nextUpdateTime != mNextUpdateTime) {
            mNextUpdateTime = nextUpdateTime;
            changed = true;
        }
        const long long nextUpdateEta = nextUpdateTime - QDateTime::currentMSecsSinceEpoch() / 1000;
        if (nextUpdateEta < 0 || nextUpdateEta > std::numeric_limits<int>::max()) {
            mNextUpdateEta = -1;
        } else {
            mNextUpdateEta = static_cast<int>(nextUpdateEta);
        }

        return {changed, announceUrlChanged};
    }
}
