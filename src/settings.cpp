// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include <QCoreApplication>
#include <QMetaEnum>
#include <QSettings>

#if QT_VERSION_MAJOR < 6
#    include <QDataStream>
#endif

#include "log/log.h"
#include "target_os.h"

#define SETTINGS_PROPERTY_DEF(type, name, key, defaultValue)                                   \
    type Settings::get_##name() const { return getValue<type>(mSettings, key, defaultValue); } \
    void Settings::set_##name(type value) {                                                    \
        if (setValue<type>(mSettings, key, std::move(value))) {                                \
            emit name##Changed();                                                              \
        }                                                                                      \
    }

namespace tremotesf {
    namespace {
        template<typename T>
        T getValue(QSettings* settings, const char* key, T defaultValue) {
            T value = settings->value(QLatin1String(key), QVariant::fromValue<T>(defaultValue)).template value<T>();
            if constexpr (std::is_enum_v<T>) {
                const auto meta = QMetaEnum::fromType<T>();
                if (!meta.valueToKey(static_cast<int>(value))) {
                    warning().log("Settings: key {} has invalid value {}, returning default value", key, value);
                    return defaultValue;
                }
            }
            return value;
        }

        template<typename T>
        bool setValue(QSettings* settings, const char* key, T value) {
            if (value != settings->value(key).template value<T>()) {
                settings->setValue(QLatin1String(key), QVariant::fromValue<T>(value));
                return true;
            }
            return false;
        }
    }

    Settings* Settings::instance() {
        static auto* const instance = new Settings(qApp);
        return instance;
    }

    SETTINGS_PROPERTY_DEF(bool, connectOnStartup, "connectOnStartup", true)
    SETTINGS_PROPERTY_DEF(bool, notificationOnDisconnecting, "notificationOnDisconnecting", true)
    SETTINGS_PROPERTY_DEF(bool, notificationOnAddingTorrent, "notificationOnAddingTorrent", true)
    SETTINGS_PROPERTY_DEF(bool, notificationOfFinishedTorrents, "notificationOfFinishedTorrents", true)
    SETTINGS_PROPERTY_DEF(
        bool, notificationsOnAddedTorrentsSinceLastConnection, "notificationsOnAddedTorrentsSinceLastConnection", false
    )
    SETTINGS_PROPERTY_DEF(
        bool,
        notificationsOnFinishedTorrentsSinceLastConnection,
        "notificationsOnFinishedTorrentsSinceLastConnection",
        false
    )

    SETTINGS_PROPERTY_DEF(bool, rememberOpenTorrentDir, "rememberOpenTorrentTorrentDir", true)
    SETTINGS_PROPERTY_DEF(QString, lastOpenTorrentDirectory, "lastOpenTorrentDirectory", {})
    SETTINGS_PROPERTY_DEF(bool, rememberAddTorrentParameters, "rememberAddTorrentParameters", true)
    SETTINGS_PROPERTY_DEF(
        TorrentData::Priority, lastAddTorrentPriority, "lastAddTorrentPriority", TorrentData::Priority::Normal
    )
    SETTINGS_PROPERTY_DEF(bool, lastAddTorrentStartAfterAdding, "lastAddTorrentStartAfterAdding", true)
    SETTINGS_PROPERTY_DEF(bool, lastAddTorrentDeleteTorrentFile, "lastAddTorrentDeleteTorrentFile", false)
    SETTINGS_PROPERTY_DEF(bool, lastAddTorrentMoveTorrentFileToTrash, "lastAddTorrentMoveTorrentFileToTrash", true)

    SETTINGS_PROPERTY_DEF(bool, fillTorrentLinkFromClipboard, "fillTorrentLinkFromClipboard", false)

    SETTINGS_PROPERTY_DEF(bool, showMainWindowWhenAddingTorrent, "showMainWindowWhenAddingTorrent", true)

    SETTINGS_PROPERTY_DEF(bool, showAddTorrentDialog, "showAddTorrentDialog", true)

    SETTINGS_PROPERTY_DEF(bool, torrentsStatusFilterEnabled, "torrentsStatusFilterEnabled", true)

    SETTINGS_PROPERTY_DEF(bool, mergeTrackersWhenAddingExistingTorrent, "mergeTrackersWhenAddingExistingTorrent", false)

    SETTINGS_PROPERTY_DEF(
        bool, askForMergingTrackersWhenAddingExistingTorrent, "askForMergingTrackersWhenAddingExistingTorrent", true
    )

    SETTINGS_PROPERTY_DEF(
        TorrentsProxyModel::StatusFilter,
        torrentsStatusFilter,
        "torrentsStatusFilter",
        TorrentsProxyModel::StatusFilter::All
    )

    SETTINGS_PROPERTY_DEF(bool, torrentsLabelFilterEnabled, "torrentsLabelFilterEnabled", true)
    SETTINGS_PROPERTY_DEF(QString, torrentsLabelFilter, "torrentsLabelFilter", {})

    SETTINGS_PROPERTY_DEF(bool, torrentsTrackerFilterEnabled, "torrentsTrackerFilterEnabled", true)
    SETTINGS_PROPERTY_DEF(QString, torrentsTrackerFilter, "torrentsTrackerFilter", {})

    SETTINGS_PROPERTY_DEF(bool, torrentsDownloadDirectoryFilterEnabled, "torrentsDownloadDirectoryFilterEnabled", true)
    SETTINGS_PROPERTY_DEF(QString, torrentsDownloadDirectoryFilter, "torrentsDownloadDirectoryFilter", {})

    SETTINGS_PROPERTY_DEF(bool, showTrayIcon, "showTrayIcon", true)
    SETTINGS_PROPERTY_DEF(Qt::ToolButtonStyle, toolButtonStyle, "toolButtonStyle", Qt::ToolButtonFollowStyle)
    SETTINGS_PROPERTY_DEF(bool, toolBarLocked, "toolBarLocked", true)
    SETTINGS_PROPERTY_DEF(bool, sideBarVisible, "sideBarVisible", true)
    SETTINGS_PROPERTY_DEF(bool, statusBarVisible, "statusBarVisible", true)
    SETTINGS_PROPERTY_DEF(bool, showTorrentPropertiesInMainWindow, "showTorrentPropertiesInMainWindow", false)

    SETTINGS_PROPERTY_DEF(QByteArray, mainWindowGeometry, "mainWindowGeometry", {})
    SETTINGS_PROPERTY_DEF(QByteArray, mainWindowState, "mainWindowState", {})
    SETTINGS_PROPERTY_DEF(QByteArray, horizontalSplitterState, "splitterState", {})
    SETTINGS_PROPERTY_DEF(QByteArray, verticalSplitterState, "verticalSplitterState", {})

    SETTINGS_PROPERTY_DEF(QByteArray, torrentsViewHeaderState, "torrentsViewHeaderState", {})
    SETTINGS_PROPERTY_DEF(QByteArray, torrentPropertiesDialogGeometry, "torrentPropertiesDialogGeometry", {})
    SETTINGS_PROPERTY_DEF(QByteArray, torrentFilesViewHeaderState, "torrentFilesViewHeaderState", {})
    SETTINGS_PROPERTY_DEF(QByteArray, trackersViewHeaderState, "trackersViewHeaderState", {})
    SETTINGS_PROPERTY_DEF(QByteArray, peersViewHeaderState, "peersViewHeaderState", {})
    SETTINGS_PROPERTY_DEF(QByteArray, localTorrentFilesViewHeaderState, "localTorrentFilesViewHeaderState", {})

    SETTINGS_PROPERTY_DEF(
        Settings::DarkThemeMode, darkThemeMode, "darkThemeMode", Settings::DarkThemeMode::FollowSystem
    )
    SETTINGS_PROPERTY_DEF(bool, useSystemAccentColor, "useSystemAccentColor", true)
    SETTINGS_PROPERTY_DEF(
        Settings::TorrentDoubleClickAction,
        torrentDoubleClickAction,
        "torrentDoubleClickAction",
        Settings::TorrentDoubleClickAction::OpenPropertiesDialog
    )

    SETTINGS_PROPERTY_DEF(bool, displayRelativeTime, "displayRelativeTime", false)

    Settings::Settings(QObject* parent) : QObject(parent) {
        if constexpr (targetOs == TargetOs::Windows) {
            mSettings = new QSettings(
                QSettings::IniFormat,
                QSettings::UserScope,
                qApp->organizationName(),
                qApp->applicationName(),
                this
            );
        } else {
            mSettings = new QSettings(this);
        }
        mSettings->setFallbacksEnabled(false);
        qRegisterMetaType<Qt::ToolButtonStyle>();
        qRegisterMetaType<TorrentData::Priority>();
        qRegisterMetaType<TorrentsProxyModel::StatusFilter>();
        qRegisterMetaType<Settings::DarkThemeMode>();
        qRegisterMetaType<Settings::TorrentDoubleClickAction>();
#if QT_VERSION_MAJOR < 6
        qRegisterMetaTypeStreamOperators<Qt::ToolButtonStyle>();
        qRegisterMetaTypeStreamOperators<TorrentData::Priority>();
        qRegisterMetaTypeStreamOperators<TorrentsProxyModel::StatusFilter>();
        qRegisterMetaTypeStreamOperators<Settings::DarkThemeMode>();
        qRegisterMetaTypeStreamOperators<Settings::TorrentDoubleClickAction>();
#endif
    }

    void Settings::sync() { mSettings->sync(); }
}
