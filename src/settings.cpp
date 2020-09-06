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

namespace tremotesf
{
    namespace
    {
        const QLatin1String connectOnStartupKey("connectOnStartup");
        const QLatin1String notificationOnDisconnectingKey("notificationOnDisconnecting");
        const QLatin1String notificationOnAddingTorrentKey("notificationOnAddingTorrent");
        const QLatin1String notificationOfFinishedTorrentsKey("notificationOfFinishedTorrents");
        const QLatin1String notificationsOnAddedTorrentsSinceLastConnectionKey("notificationsOnAddedTorrentsSinceLastConnection");
        const QLatin1String notificationsOnFinishedTorrentsSinceLastConnectionKey("notificationsOnFinishedTorrentsSinceLastConnection");
#ifdef TREMOTESF_SAILFISHOS
        const QLatin1String torrentsSortOrderKey("torrentsSortOrder");
        const QLatin1String torrentsSortRoleKey("torrentsSortRole");
#else
        const QLatin1String showTrayIconKey("showTrayIcon");
        const QLatin1String mainWindowGeometryKey("mainWindowGeometry");
        const QLatin1String mainWindowStateKey("mainWindowState");
        const QLatin1String toolButtonStyleKey("toolButtonStyle");
        const QLatin1String toolBarLockedKey("toolBarLocked");
        const QLatin1String sideBarVisibleKey("sideBarVisible");
        const QLatin1String splitterStateKey("splitterState");
        const QLatin1String statusBarVisibleKey("statusBarVisible");
        const QLatin1String torrentsViewHeaderStateKey("torrentsViewHeaderState");
        const QLatin1String torrentPropertiesDialogGeometryKey("torrentPropertiesDialogGeometry");
        const QLatin1String torrentFilesViewHeaderStateKey("torrentFilesViewHeaderState");
        const QLatin1String trackersViewHeaderStateKey("trackersViewHeaderState");
        const QLatin1String peersViewHeaderStateKey("peersViewHeaderState");
        const QLatin1String localTorrentFilesViewHeaderStateKey("localTorrentFilesViewHeaderState");
#endif
    }

    Settings* Settings::instance()
    {
        static auto* const instance = new Settings(qApp);
        return instance;
    }

    bool Settings::connectOnStartup() const
    {
        return mSettings->value(connectOnStartupKey, true).toBool();
    }

    void Settings::setConnectOnStartup(bool connect)
    {
        mSettings->setValue(connectOnStartupKey, connect);
    }

    bool Settings::notificationOnDisconnecting() const
    {
        return mSettings->value(notificationOnDisconnectingKey, true).toBool();
    }

    void Settings::setNotificationOnDisconnecting(bool enabled)
    {
        mSettings->setValue(notificationOnDisconnectingKey, enabled);
    }

    bool Settings::notificationOnAddingTorrent() const
    {
        return mSettings->value(notificationOnAddingTorrentKey, true).toBool();
    }

    void Settings::setNotificationOnAddingTorrent(bool enabled)
    {
        mSettings->setValue(notificationOnAddingTorrentKey, enabled);
    }

    bool Settings::notificationOfFinishedTorrents() const
    {
        return mSettings->value(notificationOfFinishedTorrentsKey, true).toBool();
    }

    void Settings::setNotificationOfFinishedTorrents(bool enabled)
    {
        mSettings->setValue(notificationOfFinishedTorrentsKey, enabled);
    }

    bool Settings::notificationsOnAddedTorrentsSinceLastConnection() const
    {
        return mSettings->value(notificationsOnAddedTorrentsSinceLastConnectionKey, false).toBool();
    }

    void Settings::setNotificationsOnAddedTorrentsSinceLastConnection(bool enabled)
    {
        mSettings->setValue(notificationsOnAddedTorrentsSinceLastConnectionKey, enabled);
    }

    bool Settings::notificationsOnFinishedTorrentsSinceLastConnection() const
    {
        return mSettings->value(notificationsOnFinishedTorrentsSinceLastConnectionKey, false).toBool();
    }

    void Settings::setNotificationsOnFinishedTorrentsSinceLastConnection(bool enabled)
    {
        mSettings->setValue(notificationsOnFinishedTorrentsSinceLastConnectionKey, enabled);
    }

#ifdef TREMOTESF_SAILFISHOS
    int Settings::torrentsSortOrder() const
    {
        return mSettings->value(torrentsSortOrderKey, Qt::AscendingOrder).toInt();
    }

    void Settings::setTorrentsSortOrder(int order)
    {
        mSettings->setValue(torrentsSortOrderKey, order);
    }

    int Settings::torrentsSortRole() const
    {
        return mSettings->value(torrentsSortRoleKey, TorrentsModel::NameRole).toInt();
    }

    void Settings::setTorrentsSortRole(int role)
    {
        mSettings->setValue(torrentsSortRoleKey, role);
    }
#else
    bool Settings::showTrayIcon() const
    {
        return mSettings->value(showTrayIconKey, true).toBool();
    }

    void Settings::setShowTrayIcon(bool show)
    {
        if (show != showTrayIcon()) {
            mSettings->setValue(showTrayIconKey, show);
            emit showTrayIconChanged();
        }
    }

    QByteArray Settings::mainWindowGeometry() const
    {
        return mSettings->value(mainWindowGeometryKey).toByteArray();
    }

    void Settings::setMainWindowGeometry(const QByteArray& geometry)
    {
        mSettings->setValue(mainWindowGeometryKey, geometry);
    }

    QByteArray Settings::mainWindowState() const
    {
        return mSettings->value(mainWindowStateKey).toByteArray();
    }

    void Settings::setMainWindowState(const QByteArray& state)
    {
        mSettings->setValue(mainWindowStateKey, state);
    }

    Qt::ToolButtonStyle Settings::toolButtonStyle() const
    {
        return mSettings->value(toolButtonStyleKey, Qt::ToolButtonFollowStyle).value<Qt::ToolButtonStyle>();
    }

    void Settings::setToolButtonStyle(Qt::ToolButtonStyle style)
    {
        mSettings->setValue(toolButtonStyleKey, style);
    }

    bool Settings::isToolBarLocked() const
    {
        return mSettings->value(toolBarLockedKey, true).toBool();
    }

    void Settings::setToolBarLocked(bool locked)
    {
        mSettings->setValue(toolBarLockedKey, locked);
    }

    bool Settings::isSideBarVisible() const
    {
        return mSettings->value(sideBarVisibleKey, true).toBool();
    }

    void Settings::setSideBarVisible(bool visible)
    {
        mSettings->setValue(sideBarVisibleKey, visible);
    }

    QByteArray Settings::splitterState() const
    {
        return mSettings->value(splitterStateKey).toByteArray();
    }

    void Settings::setSplitterState(const QByteArray& state)
    {
        mSettings->setValue(splitterStateKey, state);
    }

    bool Settings::isStatusBarVisible() const
    {
        return mSettings->value(statusBarVisibleKey, true).toBool();
    }

    void Settings::setStatusBarVisible(bool visible)
    {
        mSettings->setValue(statusBarVisibleKey, visible);
    }

    QByteArray Settings::torrentsViewHeaderState() const
    {
        return mSettings->value(torrentsViewHeaderStateKey).toByteArray();
    }

    void Settings::setTorrentsViewHeaderState(const QByteArray& state)
    {
        mSettings->setValue(torrentsViewHeaderStateKey, state);
    }

    QByteArray Settings::torrentPropertiesDialogGeometry() const
    {
        return mSettings->value(torrentPropertiesDialogGeometryKey).toByteArray();
    }

    void Settings::setTorrentPropertiesDialogGeometry(const QByteArray& geometry)
    {
        mSettings->setValue(torrentPropertiesDialogGeometryKey, geometry);
    }

    QByteArray Settings::torrentFilesViewHeaderState() const
    {
        return mSettings->value(torrentFilesViewHeaderStateKey).toByteArray();
    }

    void Settings::setTorrentFilesViewHeaderState(const QByteArray& state)
    {
        mSettings->setValue(torrentFilesViewHeaderStateKey, state);
    }

    QByteArray Settings::trackersViewHeaderState() const
    {
        return mSettings->value(trackersViewHeaderStateKey).toByteArray();
    }

    void Settings::setTrackersViewHeaderState(const QByteArray& state)
    {
        mSettings->setValue(trackersViewHeaderStateKey, state);
    }

    QByteArray Settings::peersViewHeaderState() const
    {
        return mSettings->value(peersViewHeaderStateKey).toByteArray();
    }

    void Settings::setPeersViewHeaderState(const QByteArray& state)
    {
        mSettings->setValue(peersViewHeaderStateKey, state);
    }

    QByteArray Settings::localTorrentFilesViewHeaderState() const
    {
        return mSettings->value(localTorrentFilesViewHeaderStateKey).toByteArray();
    }

    void Settings::setLocalTorrentFilesViewHeaderState(const QByteArray& state)
    {
        mSettings->setValue(localTorrentFilesViewHeaderStateKey, state);
    }

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
