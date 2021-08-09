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

#include "statusfilterstats.h"

#include "torrentsproxymodel.h"
#include "trpc.h"

namespace tremotesf
{
    StatusFilterStats::StatusFilterStats(Rpc* rpc, QObject* parent)
        : QObject(parent),
          mRpc(nullptr),
          mActiveTorrents(0),
          mDownloadingTorrents(0),
          mSeedingTorrents(0),
          mPausedTorrents(0),
          mCheckingTorrents(0),
          mErroredTorrents(0)
    {
        setRpc(rpc);
    }

    Rpc* StatusFilterStats::rpc() const
    {
        return mRpc;
    }

    void StatusFilterStats::setRpc(Rpc* rpc)
    {
        if (rpc != mRpc) {
            if (mRpc) {
                QObject::disconnect(mRpc, nullptr, this, nullptr);
            }
            mRpc = rpc;
            emit rpcChanged();
            if (rpc) {
                QObject::connect(rpc, &Rpc::torrentsUpdated, this, [=] {
                    mActiveTorrents = 0;
                    mDownloadingTorrents = 0;
                    mSeedingTorrents = 0;
                    mPausedTorrents = 0;
                    mCheckingTorrents = 0;
                    mErroredTorrents = 0;

                    for (const std::shared_ptr<libtremotesf::Torrent>& torrent : mRpc->torrents()) {
                        const libtremotesf::Torrent* torrentPointer = torrent.get();
                        if (TorrentsProxyModel::statusFilterAcceptsTorrent(torrentPointer, TorrentsProxyModel::Active)) {
                            ++mActiveTorrents;
                        }
                        if (TorrentsProxyModel::statusFilterAcceptsTorrent(torrentPointer, TorrentsProxyModel::Downloading)) {
                            ++mDownloadingTorrents;
                        }
                        if (TorrentsProxyModel::statusFilterAcceptsTorrent(torrentPointer, TorrentsProxyModel::Seeding)) {
                            ++mSeedingTorrents;
                        }
                        if (TorrentsProxyModel::statusFilterAcceptsTorrent(torrentPointer, TorrentsProxyModel::Paused)) {
                            ++mPausedTorrents;
                        }
                        if (TorrentsProxyModel::statusFilterAcceptsTorrent(torrentPointer, TorrentsProxyModel::Checking)) {
                            ++mCheckingTorrents;
                        }
                        if (TorrentsProxyModel::statusFilterAcceptsTorrent(torrentPointer, TorrentsProxyModel::Errored)) {
                            ++mErroredTorrents;
                        }
                    }

                    emit updated();
                });
            }
        }
    }

    int StatusFilterStats::activeTorrents() const
    {
        return mActiveTorrents;
    }

    int StatusFilterStats::downloadingTorrents() const
    {
        return mDownloadingTorrents;
    }

    int StatusFilterStats::seedingTorrents() const
    {
        return mSeedingTorrents;
    }

    int StatusFilterStats::pausedTorrents() const
    {
        return mPausedTorrents;
    }

    int StatusFilterStats::checkingTorrents() const
    {
        return mCheckingTorrents;
    }

    int StatusFilterStats::erroredTorrents() const
    {
        return mErroredTorrents;
    }
}
