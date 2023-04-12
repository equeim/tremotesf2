// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRACKERSVIEWWIDGET_H
#define TRACKERSVIEWWIDGET_H

#include <QWidget>

namespace libtremotesf {
    class Torrent;
}

namespace tremotesf {
    class BaseProxyModel;
    class BaseTreeView;
    class Rpc;
    class TrackersModel;

    class TrackersViewWidget : public QWidget {
        Q_OBJECT

    public:
        TrackersViewWidget(libtremotesf::Torrent* torrent, Rpc* rpc, QWidget* parent = nullptr);
        ~TrackersViewWidget() override;
        void setTorrent(libtremotesf::Torrent* torrent);

    private:
        void addTrackers();
        void showEditDialogs();
        void removeTrackers();

    private:
        libtremotesf::Torrent* mTorrent;
        Rpc* mRpc;
        TrackersModel* mModel;
        BaseProxyModel* mProxyModel;
        BaseTreeView* mTrackersView;
    };
}

#endif // TRACKERSVIEWWIDGET_H
