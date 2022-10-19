// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTPROPERTIESDIALOG_H
#define TREMOTESF_TORRENTPROPERTIESDIALOG_H

#include <functional>
#include <QDialog>

class QTabWidget;
class KMessageWidget;

namespace libtremotesf {
    class Torrent;
}

namespace tremotesf {
    class BaseTreeView;
    class PeersModel;
    class Rpc;
    class StringListModel;
    class TorrentFilesModel;
    class TrackersViewWidget;

    class TorrentPropertiesDialog : public QDialog {
        Q_OBJECT

    public:
        explicit TorrentPropertiesDialog(libtremotesf::Torrent* torrent, Rpc* rpc, QWidget* parent = nullptr);
        ~TorrentPropertiesDialog() override;
        QSize sizeHint() const override;

    private:
        void setupDetailsTab();
        void setupPeersTab();
        void setupWebSeedersTab();
        void setupLimitsTab();

    private:
        void setTorrent(libtremotesf::Torrent* torrent);
        void onTorrentChanged();

        libtremotesf::Torrent* mTorrent;
        Rpc* const mRpc;

        KMessageWidget* mMessageWidget;
        QTabWidget* mTabWidget;

        std::function<void()> mUpdateDetailsTab;
        TorrentFilesModel* mFilesModel;
        TrackersViewWidget* mTrackersViewWidget;
        BaseTreeView* mPeersView;
        PeersModel* mPeersModel;
        StringListModel* mWebSeedersModel;

        std::function<void()> mUpdateLimitsTab;
    };
}

#endif // TREMOTESF_TORRENTPROPERTIESDIALOG_H
