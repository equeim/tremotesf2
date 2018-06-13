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

namespace libtremotesf
{
    ServerStats::ServerStats(QObject* parent)
        : QObject(parent),
          mDownloadSpeed(0),
          mUploadSpeed(0)
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

    const SessionStats* ServerStats::currentSession() const
    {
        return &mCurrentSession;
    }

    const SessionStats* ServerStats::total() const
    {
        return &mTotal;
    }

    void ServerStats::update(const QVariantMap& serverStats)
    {
        mDownloadSpeed = serverStats[QLatin1String("downloadSpeed")].toLongLong();
        mUploadSpeed = serverStats[QLatin1String("uploadSpeed")].toLongLong();
        mCurrentSession.update(serverStats[QLatin1String("current-stats")].toMap());
        mTotal.update(serverStats[QLatin1String("cumulative-stats")].toMap());
        emit updated();
    }

    void SessionStats::update(const QVariantMap& stats)
    {
        downloaded = stats[QLatin1String("downloadedBytes")].toLongLong();
        uploaded = stats[QLatin1String("uploadedBytes")].toLongLong();
        duration = stats[QLatin1String("secondsActive")].toInt();
        sessionCount = stats[QLatin1String("sessionCount")].toInt();
    }
}
