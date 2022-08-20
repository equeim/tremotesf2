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
#include <QDataStream>
#include <QMetaEnum>
#include <QSettings>

#include "libtremotesf/println.h"
#include "libtremotesf/target_os.h"
#include "desktop/systemcolorsprovider.h"

SPECIALIZE_FORMATTER_FOR_Q_ENUM(Qt::ToolButtonStyle)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::TorrentsProxyModel::StatusFilter)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::Settings::DarkThemeMode)

#define SETTINGS_PROPERTY_DEF_IMPL(type, getter, setterType, setter, key, defaultValue) \
    type Settings::getter() const { return getValue<type>(mSettings, key, defaultValue); } \
    void Settings::setter(setterType value) { setValue<type>(mSettings, key, value); emit getter##Changed(); }

#define SETTINGS_PROPERTY_DEF_TRIVIAL(type, getter, setter, key, defaultValue) SETTINGS_PROPERTY_DEF_IMPL(type, getter, type, setter, key, defaultValue)
#define SETTINGS_PROPERTY_DEF_NON_TRIVIAL(type, getter, setter, key, defaultValue) SETTINGS_PROPERTY_DEF_IMPL(type, getter, const type&, setter, key, defaultValue)

namespace tremotesf
{
    namespace {
        template<typename T>
        T getValue(QSettings* settings, const char* key, T defaultValue) {
            const T value = settings->value(QLatin1String(key), QVariant::fromValue<T>(defaultValue)).template value<T>();
            if constexpr (std::is_enum_v<T>) {
                const auto meta = QMetaEnum::fromType<T>();
                if (!meta.valueToKey(static_cast<int>(value))) {
                    printlnWarning("Settings: key {} has invalid value {}, returning default value", key, value);
                    return defaultValue;
                }
            }
            if constexpr (std::is_same_v<T, Settings::DarkThemeMode>) {
                if (value == Settings::DarkThemeMode::FollowSystem && !SystemColorsProvider::isDarkThemeFollowSystemSupported()) {
                    printlnWarning("Settings: {} is not supported", Settings::DarkThemeMode::FollowSystem);
                    return defaultValue;
                }
            }
            return value;
        }

        template<typename T>
        void setValue(QSettings* settings, const char* key, T value) {
            settings->setValue(QLatin1String(key), QVariant::fromValue<T>(value));
        }
    }

    Settings* Settings::instance()
    {
        static auto* const instance = new Settings(qApp);
        return instance;
    }

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, connectOnStartup, setConnectOnStartup, "connectOnStartup", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationOnDisconnecting, setNotificationOnDisconnecting, "notificationOnDisconnecting", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationOnAddingTorrent, setNotificationOnAddingTorrent, "notificationOnAddingTorrent", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationOfFinishedTorrents, setNotificationOfFinishedTorrents, "notificationOfFinishedTorrents", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationsOnAddedTorrentsSinceLastConnection, setNotificationsOnAddedTorrentsSinceLastConnection, "notificationsOnAddedTorrentsSinceLastConnection", false)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, notificationsOnFinishedTorrentsSinceLastConnection, setNotificationsOnFinishedTorrentsSinceLastConnection, "notificationsOnFinishedTorrentsSinceLastConnection", false)

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isTorrentsStatusFilterEnabled, setTorrentsStatusFilterEnabled, "torrentsStatusFilterEnabled", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(tremotesf::TorrentsProxyModel::StatusFilter, torrentsStatusFilter, setTorrentsStatusFilter, "torrentsStatusFilter", TorrentsProxyModel::StatusFilter::All)

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isTorrentsTrackerFilterEnabled, setTorrentsTrackerFilterEnabled, "torrentsTrackerFilterEnabled", true)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QString, torrentsTrackerFilter, setTorrentsTrackerFilter, "torrentsTrackerFilter", {})

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isTorrentsDownloadDirectoryFilterEnabled, setTorrentsDownloadDirectoryFilterEnabled, "torrentsDownloadDirectoryFilterEnabled", true)
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QString, torrentsDownloadDirectoryFilter, setTorrentsDownloadDirectoryFilter, "torrentsDownloadDirectoryFilter", {})

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, showTrayIcon, setShowTrayIcon, "showTrayIcon", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(Qt::ToolButtonStyle, toolButtonStyle, setToolButtonStyle, "toolButtonStyle", Qt::ToolButtonFollowStyle)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isToolBarLocked, setToolBarLocked, "toolBarLocked", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isSideBarVisible, setSideBarVisible, "sideBarVisible", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isStatusBarVisible, setStatusBarVisible, "statusBarVisible", true)

    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, mainWindowGeometry, setMainWindowGeometry, "mainWindowGeometry", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, mainWindowState, setMainWindowState, "mainWindowState", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, splitterState, setSplitterState, "splitterState", {})

    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, torrentsViewHeaderState, setTorrentsViewHeaderState, "torrentsViewHeaderState", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, torrentPropertiesDialogGeometry, setTorrentPropertiesDialogGeometry, "torrentPropertiesDialogGeometry", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, torrentFilesViewHeaderState, setTorrentFilesViewHeaderState, "torrentFilesViewHeaderState", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, trackersViewHeaderState, setTrackersViewHeaderState, "trackersViewHeaderState", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, peersViewHeaderState, setPeersViewHeaderState, "peersViewHeaderState", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, localTorrentFilesViewHeaderState, setLocalTorrentFilesViewHeaderState, "localTorrentFilesViewHeaderState", {})

    namespace {
        Settings::DarkThemeMode defaultDarkThemeMode() {
            if (SystemColorsProvider::isDarkThemeFollowSystemSupported()) {
                return Settings::DarkThemeMode::FollowSystem;
            }
            return Settings::DarkThemeMode::Off;
        }

        bool defaultUseSystemAccentColor() {
            return SystemColorsProvider::isAccentColorsSupported();
        }
    }

    SETTINGS_PROPERTY_DEF_TRIVIAL(tremotesf::Settings::DarkThemeMode, darkThemeMode, setDarkThemeMode, "darkThemeMode", defaultDarkThemeMode())
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, useSystemAccentColor, setUseSystemAccentColor, "useSystemAccentColor", defaultUseSystemAccentColor());

    Settings::Settings(QObject* parent)
        : QObject(parent)
    {
        if constexpr (isTargetOsWindows) {
            mSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, qApp->organizationName(), qApp->applicationName(), this);
        } else {
            mSettings = new QSettings(this);
        }
        qRegisterMetaTypeStreamOperators<Qt::ToolButtonStyle>();
        qRegisterMetaTypeStreamOperators<TorrentsProxyModel::StatusFilter>();
        qRegisterMetaTypeStreamOperators<Settings::DarkThemeMode>();
    }
}
