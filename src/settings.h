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

#ifndef TREMOTESF_SETTINGS_H
#define TREMOTESF_SETTINGS_H

#include <QObject>

#include "torrentsproxymodel.h"

class QSettings;

#define SETTINGS_PROPERTY_IMPL(type, getter, setterType, setter) \
public: \
    Q_PROPERTY(type getter READ getter WRITE setter NOTIFY getter##Changed) \
    type getter() const; \
    void setter(setterType value); \
Q_SIGNALS: \
    void getter##Changed();

#define SETTINGS_PROPERTY_TRIVIAL(type, getter, setter) SETTINGS_PROPERTY_IMPL(type, getter, type, setter)
#define SETTINGS_PROPERTY_NON_TRIVIAL(type, getter, setter) SETTINGS_PROPERTY_IMPL(type, getter, const type&, setter)

namespace tremotesf
{
    class Settings : public QObject
    {
        Q_OBJECT

        SETTINGS_PROPERTY_TRIVIAL(bool, connectOnStartup, setConnectOnStartup)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationOnDisconnecting, setNotificationOnDisconnecting)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationOnAddingTorrent, setNotificationOnAddingTorrent)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationOfFinishedTorrents, setNotificationOfFinishedTorrents)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationsOnAddedTorrentsSinceLastConnection, setNotificationsOnAddedTorrentsSinceLastConnection)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationsOnFinishedTorrentsSinceLastConnection, setNotificationsOnFinishedTorrentsSinceLastConnection)

        SETTINGS_PROPERTY_TRIVIAL(bool, isTorrentsStatusFilterEnabled, setTorrentsStatusFilterEnabled)
        SETTINGS_PROPERTY_TRIVIAL(tremotesf::TorrentsProxyModel::StatusFilter, torrentsStatusFilter, setTorrentsStatusFilter)

        SETTINGS_PROPERTY_TRIVIAL(bool, isTorrentsTrackerFilterEnabled, setTorrentsTrackerFilterEnabled)
        SETTINGS_PROPERTY_NON_TRIVIAL(QString, torrentsTrackerFilter, setTorrentsTrackerFilter)

        SETTINGS_PROPERTY_TRIVIAL(bool, isTorrentsDownloadDirectoryFilterEnabled, setTorrentsDownloadDirectoryFilterEnabled)
        SETTINGS_PROPERTY_NON_TRIVIAL(QString, torrentsDownloadDirectoryFilter, setTorrentsDownloadDirectoryFilter)

        SETTINGS_PROPERTY_TRIVIAL(bool, showTrayIcon, setShowTrayIcon)
        SETTINGS_PROPERTY_TRIVIAL(Qt::ToolButtonStyle, toolButtonStyle, setToolButtonStyle)
        SETTINGS_PROPERTY_TRIVIAL(bool, isToolBarLocked, setToolBarLocked)
        SETTINGS_PROPERTY_TRIVIAL(bool, isSideBarVisible, setSideBarVisible)
        SETTINGS_PROPERTY_TRIVIAL(bool, isStatusBarVisible, setStatusBarVisible)

        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, mainWindowGeometry, setMainWindowGeometry)
        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, mainWindowState, setMainWindowState)
        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, splitterState, setSplitterState)

        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, torrentsViewHeaderState, setTorrentsViewHeaderState)
        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, torrentPropertiesDialogGeometry, setTorrentPropertiesDialogGeometry)
        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, torrentFilesViewHeaderState, setTorrentFilesViewHeaderState)
        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, trackersViewHeaderState, setTrackersViewHeaderState)
        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, peersViewHeaderState, setPeersViewHeaderState)
        SETTINGS_PROPERTY_NON_TRIVIAL(QByteArray, localTorrentFilesViewHeaderState, setLocalTorrentFilesViewHeaderState)

    public:
        enum class DarkThemeMode {
            FollowSystem,
            On,
            Off
        };
        Q_ENUM(DarkThemeMode)

        SETTINGS_PROPERTY_TRIVIAL(tremotesf::Settings::DarkThemeMode, darkThemeMode, setDarkThemeMode)
        SETTINGS_PROPERTY_TRIVIAL(bool, useSystemAccentColor, setUseSystemAccentColor)

    public:
        static Settings* instance();

    private:
        explicit Settings(QObject* parent = nullptr);

        QSettings* mSettings{};
    };
}

#endif // TREMOTESF_SETTINGS_H
