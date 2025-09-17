// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
// SPDX-FileCopyrightText: 2021 LuK1337
// SPDX-FileCopyrightText: 2022 Alex <tabell@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include <QCoreApplication>
#include <QMetaEnum>
#include <QSettings>

#include "log/log.h"
#include "target_os.h"

using namespace Qt::StringLiterals;

#define SETTINGS_PROPERTY_DEF(type, name, key, defaultValue)                                                 \
    namespace {                                                                                              \
        const QVariant& name##_defaultValue() {                                                              \
            static const auto v = QVariant::fromValue<type>(defaultValue);                                   \
            return v;                                                                                        \
        }                                                                                                    \
    }                                                                                                        \
    type Settings::get_##name() const { return getValue<type>(mSettings, key##_L1, name##_defaultValue()); } \
    void Settings::set_##name(type value) {                                                                  \
        if (setValue<type>(mSettings, key##_L1, std::move(value), name##_defaultValue())) {                  \
            emit name##Changed();                                                                            \
        }                                                                                                    \
    }

namespace tremotesf {
    namespace {
        template<typename T>
        T getValue(QSettings* settings, QLatin1String key, const QVariant& defaultValue) {
            T value = settings->value(key, defaultValue).value<T>();
            if constexpr (std::is_enum_v<T>) {
                const auto meta = QMetaEnum::fromType<T>();
                const auto named =
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                    meta.valueToKey(static_cast<quint64>(value));
#else
                    meta.valueToKey(static_cast<int>(value));
#endif
                if (!named) {
                    warning().log("Settings: key {} has invalid value {}, returning default value", key, value);
                    return defaultValue.value<T>();
                }
            }
            return value;
        }

        template<typename T>
        bool setValue(QSettings* settings, QLatin1String key, T newValue, const QVariant& defaultValue) {
            const auto currentValue = getValue<T>(settings, key, defaultValue);
            if (newValue != currentValue) {
                settings->setValue(key, QVariant::fromValue<T>(newValue));
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

    SETTINGS_PROPERTY_DEF(bool, displayFullDownloadDirectoryPath, "displayFullDownloadDirectoryPath", true)

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
    }

    void Settings::sync() { mSettings->sync(); }
}
