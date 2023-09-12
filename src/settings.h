// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SETTINGS_H
#define TREMOTESF_SETTINGS_H

#include <QObject>

#include "rpc/torrent.h"
#include "ui/screens/mainwindow/torrentsproxymodel.h"

class QSettings;

#define SETTINGS_PROPERTY_IMPL(type, getter, setterType, setter) \
public:                                                          \
    type getter() const;                                         \
    void setter(setterType value);                               \
Q_SIGNALS:                                                       \
    void getter##Changed();

#define SETTINGS_PROPERTY_TRIVIAL(type, getter, setter)     SETTINGS_PROPERTY_IMPL(type, getter, type, setter)
#define SETTINGS_PROPERTY_NON_TRIVIAL(type, getter, setter) SETTINGS_PROPERTY_IMPL(type, getter, const type&, setter)

namespace tremotesf {
    class Settings final : public QObject {
        Q_OBJECT

        SETTINGS_PROPERTY_TRIVIAL(bool, connectOnStartup, setConnectOnStartup)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationOnDisconnecting, setNotificationOnDisconnecting)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationOnAddingTorrent, setNotificationOnAddingTorrent)
        SETTINGS_PROPERTY_TRIVIAL(bool, notificationOfFinishedTorrents, setNotificationOfFinishedTorrents)
        SETTINGS_PROPERTY_TRIVIAL(
            bool, notificationsOnAddedTorrentsSinceLastConnection, setNotificationsOnAddedTorrentsSinceLastConnection
        )
        SETTINGS_PROPERTY_TRIVIAL(
            bool,
            notificationsOnFinishedTorrentsSinceLastConnection,
            setNotificationsOnFinishedTorrentsSinceLastConnection
        )

        SETTINGS_PROPERTY_TRIVIAL(bool, rememberOpenTorrentDir, setRememberOpenTorrentDir)
        SETTINGS_PROPERTY_NON_TRIVIAL(QString, lastOpenTorrentDirectory, setLastOpenTorrentDirectory)
        SETTINGS_PROPERTY_TRIVIAL(bool, rememberAddTorrentParameters, setRememberTorrentAddParameters)
        SETTINGS_PROPERTY_TRIVIAL(TorrentData::Priority, lastAddTorrentPriority, setLastAddTorrentPriority)
        SETTINGS_PROPERTY_TRIVIAL(bool, lastAddTorrentStartAfterAdding, setLastAddTorrentStartAfterAdding)
        SETTINGS_PROPERTY_TRIVIAL(bool, lastAddTorrentDeleteTorrentFile, setLastAddTorrentDeleteTorrentFile)

        SETTINGS_PROPERTY_TRIVIAL(bool, fillTorrentLinkFromClipboard, setFillTorrentLinkFromClipboard)

        SETTINGS_PROPERTY_TRIVIAL(bool, isTorrentsStatusFilterEnabled, setTorrentsStatusFilterEnabled)
        SETTINGS_PROPERTY_TRIVIAL(
            TorrentsProxyModel::StatusFilter, torrentsStatusFilter, setTorrentsStatusFilter
        )

        SETTINGS_PROPERTY_TRIVIAL(bool, isTorrentsTrackerFilterEnabled, setTorrentsTrackerFilterEnabled)
        SETTINGS_PROPERTY_NON_TRIVIAL(QString, torrentsTrackerFilter, setTorrentsTrackerFilter)

        SETTINGS_PROPERTY_TRIVIAL(
            bool, isTorrentsDownloadDirectoryFilterEnabled, setTorrentsDownloadDirectoryFilterEnabled
        )
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
        enum class DarkThemeMode { FollowSystem, On, Off };
        Q_ENUM(DarkThemeMode)

        SETTINGS_PROPERTY_TRIVIAL(Settings::DarkThemeMode, darkThemeMode, setDarkThemeMode)
        SETTINGS_PROPERTY_TRIVIAL(bool, useSystemAccentColor, setUseSystemAccentColor)

    public:
        static Settings* instance();

    private:
        explicit Settings(QObject* parent = nullptr);

        QSettings* mSettings{};
    };
}

#endif // TREMOTESF_SETTINGS_H
