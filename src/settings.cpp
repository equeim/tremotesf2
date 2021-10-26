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

#include "settings.h"

#include <QCoreApplication>
#include <QSettings>

#include "torrentsmodel.h"

#define SETTINGS_PROPERTY_DEF_IMPL(type, getter, setterType, setter, key, defaultValue, convert) \
    type Settings::getter() const { return mSettings->value(QLatin1String(key), defaultValue).convert(); } \
    void Settings::setter(setterType value) { mSettings->setValue(QLatin1String(key), value); emit getter##Changed(); }

#define SETTINGS_PROPERTY_DEF_TRIVIAL(type, getter, setter, key, defaultValue, convert) SETTINGS_PROPERTY_DEF_IMPL(type, getter, type, setter, key, defaultValue, convert)
#define SETTINGS_PROPERTY_DEF_NON_TRIVIAL(type, getter, setter, key, defaultValue, convert) SETTINGS_PROPERTY_DEF_IMPL(type, getter, const type&, setter, key, defaultValue, convert)

namespace tremotesf
{
    Settings* Settings::instance()
    {
        static auto* const instance = new Settings(qApp);
        return instance;
    }

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, connectOnStartup, setConnectOnStartup, "connectOnStartup", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationOnDisconnecting, setNotificationOnDisconnecting, "notificationOnDisconnecting", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationOnAddingTorrent, setNotificationOnAddingTorrent, "notificationOnAddingTorrent", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationOfFinishedTorrents, setNotificationOfFinishedTorrents, "notificationOfFinishedTorrents", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationsOnAddedTorrentsSinceLastConnection, setNotificationsOnAddedTorrentsSinceLastConnection, "notificationsOnAddedTorrentsSinceLastConnection", false, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationsOnFinishedTorrentsSinceLastConnection, setNotificationsOnFinishedTorrentsSinceLastConnection, "notificationsOnFinishedTorrentsSinceLastConnection", false, toBool)

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isTorrentsStatusFilterEnabled, setTorrentsStatusFilterEnabled, "torrentsStatusFilterEnabled", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(tremotesf::TorrentsProxyModel::StatusFilter, torrentsStatusFilter, setTorrentsStatusFilter, "torrentsStatusFilter", TorrentsProxyModel::StatusFilter::All, value<TorrentsProxyModel::StatusFilter>);

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isTorrentsTrackerFilterEnabled, setTorrentsTrackerFilterEnabled, "torrentsTrackerFilterEnabled", true, toBool)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QString, torrentsTrackerFilter, setTorrentsTrackerFilter, "torrentsTrackerFilter", {}, toString)

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isTorrentsDownloadDirectoryFilterEnabled, setTorrentsDownloadDirectoryFilterEnabled, "torrentsDownloadDirectoryFilterEnabled", true, toBool)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QString, torrentsDownloadDirectoryFilter, setTorrentsDownloadDirectoryFilter, "torrentsDownloadDirectoryFilter", {}, toString)

#ifdef TREMOTESF_SAILFISHOS
    SETTINGS_PROPERTY_DEF_TRIVIAL(int, torrentsSortOrder, setTorrentsSortOrder, "torrentsSortOrder", Qt::AscendingOrder, toInt)
    SETTINGS_PROPERTY_DEF_TRIVIAL(int, torrentsSortRole, setTorrentsSortRole, "torrentsSortRole", TorrentsModel::NameRole, toInt)
#else
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, showTrayIcon, setShowTrayIcon, "showTrayIcon", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(Qt::ToolButtonStyle, toolButtonStyle, setToolButtonStyle, "toolButtonStyle", Qt::ToolButtonFollowStyle, value<Qt::ToolButtonStyle>)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isToolBarLocked, setToolBarLocked, "toolBarLocked", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isSideBarVisible, setSideBarVisible, "sideBarVisible", true, toBool)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isStatusBarVisible, setStatusBarVisible, "statusBarVisible", true, toBool)

    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, mainWindowGeometry, setMainWindowGeometry, "mainWindowGeometry", {}, toByteArray)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, mainWindowState, setMainWindowState, "mainWindowState", {}, toByteArray)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, splitterState, setSplitterState, "splitterState", {}, toByteArray)

    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, torrentsViewHeaderState, setTorrentsViewHeaderState, "torrentsViewHeaderState", {}, toByteArray)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, torrentPropertiesDialogGeometry, setTorrentPropertiesDialogGeometry, "torrentPropertiesDialogGeometry", {}, toByteArray)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, torrentFilesViewHeaderState, setTorrentFilesViewHeaderState, "torrentFilesViewHeaderState", {}, toByteArray)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, trackersViewHeaderState, setTrackersViewHeaderState, "trackersViewHeaderState", {}, toByteArray)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, peersViewHeaderState, setPeersViewHeaderState, "peersViewHeaderState", {}, toByteArray)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, localTorrentFilesViewHeaderState, setLocalTorrentFilesViewHeaderState, "localTorrentFilesViewHeaderState", {}, toByteArray)
#endif

    Settings::Settings(QObject* parent)
        : QObject(parent),
#ifdef Q_OS_WIN
          mSettings(new QSettings(QSettings::IniFormat, QSettings::UserScope, qApp->organizationName(), qApp->applicationName(), this))
#else
          mSettings(new QSettings(this))
#endif
    {
    }
}
