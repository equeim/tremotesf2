// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRACKERSVIEWWIDGET_H
#define TRACKERSVIEWWIDGET_H

#include <QWidget>

namespace tremotesf {
    class Torrent;
}

namespace tremotesf {
    class BaseProxyModel;
    class BaseTreeView;
    class Rpc;
    class TrackersModel;

    class TrackersViewWidget final : public QWidget {
        Q_OBJECT

    public:
        TrackersViewWidget(Rpc* rpc, QWidget* parent = nullptr);
        Q_DISABLE_COPY_MOVE(TrackersViewWidget)

        void setTorrent(Torrent* torrent, bool oldTorrentDestroyed);
        void saveState();

    private:
        void addTrackers();
        void showEditDialogs();
        void removeTrackers();

        Torrent* mTorrent{};
        Rpc* mRpc{};
        TrackersModel* mModel{};
        BaseProxyModel* mProxyModel{};
        BaseTreeView* mTrackersView{};
    };
}

#endif // TRACKERSVIEWWIDGET_H
