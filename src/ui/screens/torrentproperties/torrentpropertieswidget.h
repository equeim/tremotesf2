// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTPROPERTIESWIDGET_H
#define TREMOTESF_TORRENTPROPERTIESWIDGET_H

#include <functional>

#include <QTabWidget>

namespace tremotesf {
    class BaseTreeView;
    class PeersModel;
    class Rpc;
    class StringListModel;
    class TorrentFilesModel;
    class TorrentFilesView;
    class Torrent;
    class TrackersViewWidget;

    class TorrentPropertiesWidget : public QTabWidget {
        Q_OBJECT
    public:
        explicit TorrentPropertiesWidget(Rpc* rpc, bool horizontalDetails, QWidget* parent = nullptr);

        void setTorrent(Torrent* torrent);
        bool hasTorrent() const { return mTorrent != nullptr; }
        void saveState();

    private:
        void setupDetailsTab(bool horizontal);
        void setupPeersTab();
        void setupWebSeedersTab();
        void setupLimitsTab();

        void setTorrent(Torrent* torrent, bool oldTorrentDestroyed);

        Torrent* mTorrent{};
        Rpc* const mRpc{};

        std::function<void()> mUpdateDetailsTab;
        TorrentFilesModel* mFilesModel{};
        TorrentFilesView* mFilesView{};
        TrackersViewWidget* mTrackersViewWidget{};
        BaseTreeView* mPeersView{};
        PeersModel* mPeersModel{};
        StringListModel* mWebSeedersModel{};

        bool mUpdatingLimits{};
        std::function<void()> mUpdateLimitsTab;
    signals:
        void hasTorrentChanged(bool hasTorrent);
    };
}

#endif // TREMOTESF_TORRENTPROPERTIESWIDGET_H
