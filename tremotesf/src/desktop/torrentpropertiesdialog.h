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

#ifndef TREMOTESF_TORRENTPROPERTIESDIALOG_H
#define TREMOTESF_TORRENTPROPERTIESDIALOG_H

#include <functional>
#include <QDialog>

class QTabWidget;
class KMessageWidget;

namespace libtremotesf
{
    class Torrent;
}

namespace tremotesf
{
    using libtremotesf::Torrent;
    class BaseTreeView;
    class PeersModel;
    class Rpc;
    class TorrentFilesModel;
    class TrackersViewWidget;

    class TorrentPropertiesDialog : public QDialog
    {
    public:
        explicit TorrentPropertiesDialog(Torrent* torrent, Rpc* rpc, QWidget* parent = nullptr);
        ~TorrentPropertiesDialog() override;
        QSize sizeHint() const override;

    private:
        void setupDetailsTab();
        void setupPeersTab();
        void setupLimitsTab();

    private:
        void setTorrent(Torrent* torrent);

        Torrent* mTorrent;
        Rpc* const mRpc;

        KMessageWidget* mMessageWidget;
        QTabWidget* mTabWidget;

        std::function<void()> mUpdateDetailsTab;
        TorrentFilesModel* mFilesModel;
        TrackersViewWidget* mTrackersViewWidget;
        BaseTreeView* mPeersView;
        PeersModel* mPeersModel;

        std::function<void()> mUpdateLimitsTab;
    };
}

#endif // TREMOTESF_TORRENTPROPERTIESDIALOG_H
