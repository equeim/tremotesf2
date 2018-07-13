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

#include "serverstats.h"
#include "rpc.h"

#include <QJsonObject>

namespace libtremotesf
{
    ServerStats::ServerStats(QObject* parent)
        : QObject(parent),
          mDownloadSpeed(0),
          mUploadSpeed(0),
          mCurrentSession(new SessionStats(this)),
          mTotal(new SessionStats(this))
    {

    }

    long long ServerStats::downloadSpeed() const
    {
        return mDownloadSpeed;
    }

    long long ServerStats::uploadSpeed() const
    {
        return mUploadSpeed;
    }

    SessionStats* ServerStats::currentSession() const
    {
        return mCurrentSession;
    }

    SessionStats* ServerStats::total() const
    {
        return mTotal;
    }

    void ServerStats::update(const QJsonObject& serverStats)
    {
        mDownloadSpeed = serverStats[QLatin1String("downloadSpeed")].toDouble();
        mUploadSpeed = serverStats[QLatin1String("uploadSpeed")].toDouble();
        mCurrentSession->update(serverStats[QLatin1String("current-stats")].toObject());
        mTotal->update(serverStats[QLatin1String("cumulative-stats")].toObject());
        emit updated();
    }

    SessionStats::SessionStats(QObject* parent)
        : QObject(parent)
    {

    }

    long long SessionStats::downloaded() const
    {
        return mDownloaded;
    }

    long long SessionStats::uploaded() const
    {
        return mUploaded;
    }

    int SessionStats::duration() const
    {
        return mDuration;
    }

    int SessionStats::sessionCount() const
    {
        return mSessionCount;
    }

    void SessionStats::update(const QJsonObject& stats)
    {
        mDownloaded = stats[QLatin1String("downloadedBytes")].toDouble();
        mUploaded = stats[QLatin1String("uploadedBytes")].toDouble();
        mDuration = stats[QLatin1String("secondsActive")].toInt();
        mSessionCount = stats[QLatin1String("sessionCount")].toInt();
        emit updated();
    }
}
