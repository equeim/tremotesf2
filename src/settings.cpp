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

#include "settings.h"

#include <QCoreApplication>
#include <QSettings>

#include "torrentsmodel.h"

namespace tremotesf
{
    namespace
    {
        Settings* instancePointer = nullptr;

        const QString connectOnStartupKey("connectOnStartup");
        const QString notificationOnDisconnectingKey("notificationOnDisconnecting");
        const QString notificationOnAddingTorrentKey("notificationOnAddingTorrent");
        const QString notificationOfFinishedTorrentsKey("notificationOfFinishedTorrents");
#ifdef TREMOTESF_SAILFISHOS
        const QString torrentsSortOrderKey("torrentsSortOrder");
        const QString torrentsSortRoleKey("torrentsSortRole");
#else
        const QString showTrayIconKey("showTrayIcon");
        const QString startMinimizedKey("startMinimized");
        const QString mainWindowGeometryKey("mainWindowGeometry");
        const QString toolButtonStyleKey("toolButtonStyle");
        const QString toolBarVisibleKey("toolBarVisible");
        const QString toolBarAreaKey("toolBarArea");
        const QString sideBarVisibleKey("sideBarVisible");
        const QString splitterStateKey("splitterState");
        const QString statusBarVisibleKey("statusBarVisible");
        const QString localTorrentFilesViewHeaderStateKey("localTorrentFilesViewHeaderState");
        const QString torrentsViewHeaderStateKey("torrentsViewHeaderState");
        const QString torrentFilesViewHeaderStateKey("torrentFilesViewHeaderState");
        const QString trackersViewHeaderStateKey("trackersViewHeaderState");
        const QString peersViewHeaderStateKey("peersViewHeaderState");
#endif
    }

    Settings* Settings::instance()
    {
        if (!instancePointer) {
            instancePointer = new Settings(qApp);
        }
        return instancePointer;
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

    bool Settings::startMinimized() const
    {
        return mSettings->value(startMinimizedKey, false).toBool();
    }

    void Settings::setStartMinimized(bool minimized)
    {
        mSettings->setValue(startMinimizedKey, minimized);
    }

    QByteArray Settings::mainWindowGeometry() const
    {
        return mSettings->value(mainWindowGeometryKey).toByteArray();
    }

    void Settings::setMainWindowGeometry(const QByteArray& geometry)
    {
        mSettings->setValue(mainWindowGeometryKey, geometry);
    }

    Qt::ToolButtonStyle Settings::toolButtonStyle() const
    {
        return mSettings->value(toolButtonStyleKey, Qt::ToolButtonFollowStyle).value<Qt::ToolButtonStyle>();
    }

    void Settings::setToolButtonStyle(Qt::ToolButtonStyle style)
    {
        mSettings->setValue(toolButtonStyleKey, style);
    }

    bool Settings::isToolBarVisible() const
    {
        return mSettings->value(toolBarVisibleKey, true).toBool();
    }

    void Settings::setToolBarVisible(bool visible)
    {
        mSettings->setValue(toolBarVisibleKey, visible);
    }

    Qt::ToolBarArea Settings::toolBarArea() const
    {
        return mSettings->value(toolBarAreaKey, Qt::TopToolBarArea).value<Qt::ToolBarArea>();
    }

    void Settings::setToolBarArea(Qt::ToolBarArea area)
    {
        mSettings->setValue(toolBarAreaKey, area);
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

    QByteArray Settings::localTorrentFilesViewHeaderState() const
    {
        return mSettings->value(localTorrentFilesViewHeaderStateKey).toByteArray();
    }

    void Settings::setLocalTorrentFilesViewHeaderState(const QByteArray& state)
    {
        mSettings->setValue(localTorrentFilesViewHeaderStateKey, state);
    }

    QByteArray Settings::torrentsViewHeaderState() const
    {
        return mSettings->value(torrentsViewHeaderStateKey).toByteArray();
    }

    void Settings::setTorrentsViewHeaderState(const QByteArray& state)
    {
        mSettings->setValue(torrentsViewHeaderStateKey, state);
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
