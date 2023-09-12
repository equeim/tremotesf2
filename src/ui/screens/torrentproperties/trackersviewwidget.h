// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
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
        TrackersViewWidget(Torrent* torrent, Rpc* rpc, QWidget* parent = nullptr);
        ~TrackersViewWidget() override;
        Q_DISABLE_COPY_MOVE(TrackersViewWidget)

        void setTorrent(Torrent* torrent);

    private:
        void addTrackers();
        void showEditDialogs();
        void removeTrackers();

        Torrent* mTorrent;
        Rpc* mRpc;
        TrackersModel* mModel;
        BaseProxyModel* mProxyModel;
        BaseTreeView* mTrackersView;
    };
}

#endif // TRACKERSVIEWWIDGET_H
