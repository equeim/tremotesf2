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

#ifndef TREMOTESF_SERVERSTATS_H
#define TREMOTESF_SERVERSTATS_H

#include <QObject>
#include <QVariantMap>

namespace libtremotesf
{
    class Rpc;

    class SessionStats : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(long long downloaded READ downloaded NOTIFY updated)
        Q_PROPERTY(long long uploaded READ uploaded NOTIFY updated)
        Q_PROPERTY(int duration READ duration NOTIFY updated)
        Q_PROPERTY(int sessionCount READ sessionCount NOTIFY updated)
    public:
        explicit SessionStats(QObject* parent = nullptr);

        long long downloaded() const;
        long long uploaded() const;
        int duration() const;
        int sessionCount() const;

        void update(const QVariantMap& stats);

    private:
        long long mDownloaded;
        long long mUploaded;
        int mDuration;
        int mSessionCount;

    signals:
        void updated();
    };

    class ServerStats : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(long long downloadSpeed READ downloadSpeed NOTIFY updated)
        Q_PROPERTY(long long uploadSpeed READ uploadSpeed NOTIFY updated)
        Q_PROPERTY(libtremotesf::SessionStats* currentSession READ currentSession CONSTANT)
        Q_PROPERTY(libtremotesf::SessionStats* total READ total CONSTANT)
    public:
        explicit ServerStats(QObject* parent);

        long long downloadSpeed() const;
        long long uploadSpeed() const;

        SessionStats* currentSession() const;
        SessionStats* total() const;

        void update(const QVariantMap& serverStats);

    private:
        long long mDownloadSpeed;
        long long mUploadSpeed;
        SessionStats* mCurrentSession;
        SessionStats* mTotal;
    signals:
        void updated();
    };
}

#endif // TREMOTESF_SERVERSTATS_H
