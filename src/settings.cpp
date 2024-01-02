// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QMetaEnum>
#include <QSettings>

#include "log/log.h"
#include "target_os.h"

SPECIALIZE_FORMATTER_FOR_Q_ENUM(Qt::ToolButtonStyle)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::TorrentsProxyModel::StatusFilter)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::Settings::DarkThemeMode)
SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::Settings::TorrentDoubleClickAction)

#define SETTINGS_PROPERTY_DEF_IMPL(type, getter, setterType, setter, key, defaultValue)    \
    type Settings::getter() const { return getValue<type>(mSettings, key, defaultValue); } \
    void Settings::setter(setterType value) {                                              \
        setValue<type>(mSettings, key, value);                                             \
        emit getter##Changed();                                                            \
    }

#define SETTINGS_PROPERTY_DEF_TRIVIAL(type, getter, setter, key, defaultValue) \
    SETTINGS_PROPERTY_DEF_IMPL(type, getter, type, setter, key, defaultValue)
#define SETTINGS_PROPERTY_DEF_NON_TRIVIAL(type, getter, setter, key, defaultValue) \
    SETTINGS_PROPERTY_DEF_IMPL(type, getter, const type&, setter, key, defaultValue)

namespace tremotesf {
    namespace {
        template<typename T>
        T getValue(QSettings* settings, const char* key, T defaultValue) {
            T value = settings->value(QLatin1String(key), QVariant::fromValue<T>(defaultValue)).template value<T>();
            if constexpr (std::is_enum_v<T>) {
                const auto meta = QMetaEnum::fromType<T>();
                if (!meta.valueToKey(static_cast<int>(value))) {
                    logWarning("Settings: key {} has invalid value {}, returning default value", key, value);
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

    Settings* Settings::instance() {
        static auto* const instance = new Settings(qApp);
        return instance;
    }

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, connectOnStartup, setConnectOnStartup, "connectOnStartup", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, notificationOnDisconnecting, setNotificationOnDisconnecting, "notificationOnDisconnecting", true
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, notificationOnAddingTorrent, setNotificationOnAddingTorrent, "notificationOnAddingTorrent", true
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, notificationOfFinishedTorrents, setNotificationOfFinishedTorrents, "notificationOfFinishedTorrents", true
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool,
        notificationsOnAddedTorrentsSinceLastConnection,
        setNotificationsOnAddedTorrentsSinceLastConnection,
        "notificationsOnAddedTorrentsSinceLastConnection",
        false
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool,
        notificationsOnFinishedTorrentsSinceLastConnection,
        setNotificationsOnFinishedTorrentsSinceLastConnection,
        "notificationsOnFinishedTorrentsSinceLastConnection",
        false
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, rememberOpenTorrentDir, setRememberOpenTorrentDir, "rememberOpenTorrentTorrentDir", true
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QString, lastOpenTorrentDirectory, setLastOpenTorrentDirectory, "lastOpenTorrentDirectory", {}
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, rememberAddTorrentParameters, setRememberTorrentAddParameters, "rememberAddTorrentParameters", true
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        TorrentData::Priority,
        lastAddTorrentPriority,
        setLastAddTorrentPriority,
        "lastAddTorrentPriority",
        TorrentData::Priority::Normal
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, lastAddTorrentStartAfterAdding, setLastAddTorrentStartAfterAdding, "lastAddTorrentStartAfterAdding", true
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool,
        lastAddTorrentDeleteTorrentFile,
        setLastAddTorrentDeleteTorrentFile,
        "lastAddTorrentDeleteTorrentFile",
        false
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool,
        lastAddTorrentMoveTorrentFileToTrash,
        setLastAddTorrentMoveTorrentFileToTrash,
        "lastAddTorrentMoveTorrentFileToTrash",
        true
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, fillTorrentLinkFromClipboard, setFillTorrentLinkFromClipboard, "fillTorrentLinkFromClipboard", false
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool,
        showMainWindowWhenAddingTorrent,
        setShowMainWindowWhenAddingTorrent,
        "showMainWindowWhenAddingTorrent",
        true
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, showAddTorrentDialog, setShowAddTorrentDialog, "showAddTorrentDialog", true)

    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, isTorrentsStatusFilterEnabled, setTorrentsStatusFilterEnabled, "torrentsStatusFilterEnabled", true
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        TorrentsProxyModel::StatusFilter,
        torrentsStatusFilter,
        setTorrentsStatusFilter,
        "torrentsStatusFilter",
        TorrentsProxyModel::StatusFilter::All
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool, isTorrentsTrackerFilterEnabled, setTorrentsTrackerFilterEnabled, "torrentsTrackerFilterEnabled", true
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QString, torrentsTrackerFilter, setTorrentsTrackerFilter, "torrentsTrackerFilter", {}
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(
        bool,
        isTorrentsDownloadDirectoryFilterEnabled,
        setTorrentsDownloadDirectoryFilterEnabled,
        "torrentsDownloadDirectoryFilterEnabled",
        true
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QString,
        torrentsDownloadDirectoryFilter,
        setTorrentsDownloadDirectoryFilter,
        "torrentsDownloadDirectoryFilter",
        {}
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, showTrayIcon, setShowTrayIcon, "showTrayIcon", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        Qt::ToolButtonStyle, toolButtonStyle, setToolButtonStyle, "toolButtonStyle", Qt::ToolButtonFollowStyle
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isToolBarLocked, setToolBarLocked, "toolBarLocked", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isSideBarVisible, setSideBarVisible, "sideBarVisible", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, isStatusBarVisible, setStatusBarVisible, "statusBarVisible", true)

    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, mainWindowGeometry, setMainWindowGeometry, "mainWindowGeometry", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, mainWindowState, setMainWindowState, "mainWindowState", {})
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(QByteArray, splitterState, setSplitterState, "splitterState", {})

    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QByteArray, torrentsViewHeaderState, setTorrentsViewHeaderState, "torrentsViewHeaderState", {}
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QByteArray,
        torrentPropertiesDialogGeometry,
        setTorrentPropertiesDialogGeometry,
        "torrentPropertiesDialogGeometry",
        {}
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QByteArray, torrentFilesViewHeaderState, setTorrentFilesViewHeaderState, "torrentFilesViewHeaderState", {}
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QByteArray, trackersViewHeaderState, setTrackersViewHeaderState, "trackersViewHeaderState", {}
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QByteArray, peersViewHeaderState, setPeersViewHeaderState, "peersViewHeaderState", {}
    )
    SETTINGS_PROPERTY_DEF_NON_TRIVIAL(
        QByteArray,
        localTorrentFilesViewHeaderState,
        setLocalTorrentFilesViewHeaderState,
        "localTorrentFilesViewHeaderState",
        {}
    )

    SETTINGS_PROPERTY_DEF_TRIVIAL(
        Settings::DarkThemeMode, darkThemeMode, setDarkThemeMode, "darkThemeMode", Settings::DarkThemeMode::FollowSystem
    )
    SETTINGS_PROPERTY_DEF_TRIVIAL(bool, useSystemAccentColor, setUseSystemAccentColor, "useSystemAccentColor", true)
    SETTINGS_PROPERTY_DEF_TRIVIAL(
        Settings::TorrentDoubleClickAction,
        torrentDoubleClickAction,
        setTorrentDoubleClickAction,
        "torrentDoubleClickAction",
        Settings::TorrentDoubleClickAction::OpenPropertiesDialog
    )

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
