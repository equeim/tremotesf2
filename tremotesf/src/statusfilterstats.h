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

#ifndef TREMOTESF_STATUSFILTERSTATS_H
#define TREMOTESF_STATUSFILTERSTATS_H

#include <QObject>

namespace libtremotesf
{
    class Torrent;
}

namespace tremotesf
{
    using libtremotesf::Torrent;
    class Rpc;

    class StatusFilterStats : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(tremotesf::Rpc* rpc READ rpc WRITE setRpc)
        Q_PROPERTY(int activeTorrents READ activeTorrents NOTIFY updated)
        Q_PROPERTY(int downloadingTorrents READ downloadingTorrents NOTIFY updated)
        Q_PROPERTY(int seedingTorrents READ seedingTorrents NOTIFY updated)
        Q_PROPERTY(int pausedTorrents READ pausedTorrents NOTIFY updated)
        Q_PROPERTY(int checkingTorrents READ checkingTorrents NOTIFY updated)
        Q_PROPERTY(int erroredTorrents READ erroredTorrents NOTIFY updated)
    public:
        explicit StatusFilterStats(Rpc* rpc = nullptr, QObject* parent = nullptr);

        Rpc* rpc() const;
        void setRpc(Rpc* rpc);

        int activeTorrents() const;
        int downloadingTorrents() const;
        int seedingTorrents() const;
        int pausedTorrents() const;
        int checkingTorrents() const;
        int erroredTorrents() const;

    private:
        Rpc* mRpc;
        int mActiveTorrents;
        int mDownloadingTorrents;
        int mSeedingTorrents;
        int mPausedTorrents;
        int mCheckingTorrents;
        int mErroredTorrents;
    signals:
        void updated();
    };
}

#endif // TREMOTESF_STATUSFILTERSTATS_H
