/*
 * Tremotesf
 * Copyright (C) 2015-2016 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_SETTINGS_H
#define TREMOTESF_SETTINGS_H

#include <QObject>

class QSettings;

namespace tremotesf
{
    class Settings : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool connectOnStartup READ connectOnStartup WRITE setConnectOnStartup)
        Q_PROPERTY(bool notificationOnDisconnecting READ notificationOnDisconnecting WRITE setNotificationOnDisconnecting)
        Q_PROPERTY(bool notificationOnAddingTorrent READ notificationOnAddingTorrent WRITE setNotificationOnAddingTorrent)
        Q_PROPERTY(bool notificationOfFinishedTorrents READ notificationOfFinishedTorrents WRITE setNotificationOfFinishedTorrents)
    public:
        static Settings* instance();

        bool connectOnStartup() const;
        void setConnectOnStartup(bool connect);

        bool notificationOnDisconnecting() const;
        void setNotificationOnDisconnecting(bool enabled);

        bool notificationOnAddingTorrent() const;
        void setNotificationOnAddingTorrent(bool enabled);

        bool notificationOfFinishedTorrents() const;
        void setNotificationOfFinishedTorrents(bool enabled);

#ifdef TREMOTESF_SAILFISHOS
        Q_PROPERTY(int torrentsSortOrder READ torrentsSortOrder WRITE setTorrentsSortOrder)
        int torrentsSortOrder() const;
        void setTorrentsSortOrder(int order);

        Q_PROPERTY(int torrentsSortRole READ torrentsSortRole WRITE setTorrentsSortRole)
        int torrentsSortRole() const;
        void setTorrentsSortRole(int role);
#else
        bool showTrayIcon() const;
        void setShowTrayIcon(bool show);

        bool startMinimized() const;
        void setStartMinimized(bool minimized);

        QByteArray mainWindowGeometry() const;
        void setMainWindowGeometry(const QByteArray& geometry);

        Qt::ToolButtonStyle toolButtonStyle() const;
        void setToolButtonStyle(Qt::ToolButtonStyle style);

        bool isToolBarVisible() const;
        void setToolBarVisible(bool visible);

        Qt::ToolBarArea toolBarArea() const;
        void setToolBarArea(Qt::ToolBarArea area);

        bool isSideBarVisible() const;
        void setSideBarVisible(bool visible);

        QByteArray splitterState() const;
        void setSplitterState(const QByteArray& state);

        bool isStatusBarVisible() const;
        void setStatusBarVisible(bool visible);

        QByteArray localTorrentFilesViewHeaderState() const;
        void setLocalTorrentFilesViewHeaderState(const QByteArray& state);

        QByteArray torrentsViewHeaderState() const;
        void setTorrentsViewHeaderState(const QByteArray& state);

        QByteArray torrentFilesViewHeaderState() const;
        void setTorrentFilesViewHeaderState(const QByteArray& state);

        QByteArray trackersViewHeaderState() const;
        void setTrackersViewHeaderState(const QByteArray& state);

        QByteArray peersViewHeaderState() const;
        void setPeersViewHeaderState(const QByteArray& state);
#endif
    private:
        explicit Settings(QObject* parent = nullptr);

        QSettings* mSettings;

#ifndef TREMOTESF_SAILFISHOS
    signals:
        void showTrayIconChanged();
#endif
    };
}

#endif // TREMOTESF_SETTINGS_H
