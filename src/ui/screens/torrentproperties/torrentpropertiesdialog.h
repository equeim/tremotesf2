// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_TORRENTPROPERTIESDIALOG_H
#define TREMOTESF_TORRENTPROPERTIESDIALOG_H

#include <functional>
#include <QDialog>
#include <QPointer>

#include "ui/savewindowstatedispatcher.h"

class QTabWidget;
class KMessageWidget;

namespace tremotesf {
    class Torrent;
}

namespace tremotesf {
    class BaseTreeView;
    class PeersModel;
    class Rpc;
    class StringListModel;
    class TorrentFilesModel;
    class TorrentFilesView;
    class TrackersViewWidget;

    class TorrentPropertiesDialog final : public QDialog {
        Q_OBJECT

    public:
        explicit TorrentPropertiesDialog(Torrent* torrent, Rpc* rpc, QWidget* parent = nullptr);
        Q_DISABLE_COPY_MOVE(TorrentPropertiesDialog)

        QSize sizeHint() const override;

    private:
        void setupDetailsTab();
        void setupPeersTab();
        void setupWebSeedersTab();
        void setupLimitsTab();

        void setTorrent(Torrent* torrent);
        void onTorrentChanged();

        void saveState();

        QPointer<Torrent> mTorrent;
        Rpc* const mRpc;

        KMessageWidget* mMessageWidget;
        QTabWidget* mTabWidget;

        std::function<void()> mUpdateDetailsTab;
        TorrentFilesModel* mFilesModel;
        TorrentFilesView* mFilesView;
        TrackersViewWidget* mTrackersViewWidget;
        BaseTreeView* mPeersView;
        PeersModel* mPeersModel;
        StringListModel* mWebSeedersModel;

        bool mUpdatingLimits{};
        std::function<void()> mUpdateLimitsTab;

        SaveWindowStateHandler mSaveStateHandler{this, [this] { saveState(); }};
    };
}

#endif // TREMOTESF_TORRENTPROPERTIESDIALOG_H
