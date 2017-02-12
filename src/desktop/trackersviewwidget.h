/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#ifndef TRACKERSVIEWWIDGET_H
#define TRACKERSVIEWWIDGET_H

#include <QWidget>

namespace tremotesf
{
    class BaseProxyModel;
    class BaseTreeView;
    class Torrent;
    class TrackersModel;

    class TrackersViewWidget : public QWidget
    {
    public:
        explicit TrackersViewWidget(Torrent* torrent, QWidget* parent = nullptr);
        ~TrackersViewWidget() override;
        void setTorrent(Torrent* torrent);

    private:
        void addTracker();
        void showEditDialogs();
        void removeTrackers();

    private:
        Torrent* mTorrent;
        TrackersModel* mModel;
        BaseProxyModel* mProxyModel;
        BaseTreeView* mTrackersView;
    };
}

#endif // TRACKERSVIEWWIDGET_H
