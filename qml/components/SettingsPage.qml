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

import Sailfish.Silica 1.0
import QtQuick 2.2

import harbour.tremotesf 1.0

Page {
    allowedOrientations: defaultAllowedOrientations

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTranslate("tremotesf", "Settings")
            }

            TextSwitch {
                text: qsTranslate("tremotesf", "Connect to server on startup")
                onClicked: Settings.connectOnStartup = checked
                Component.onCompleted: checked = Settings.connectOnStartup
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Notifications")
            }

            TextSwitch {
                text: qsTranslate("tremotesf", "Notify when disconnecting from server")
                onClicked: Settings.notificationOnDisconnecting = checked
                Component.onCompleted: checked = Settings.notificationOnDisconnecting
            }

            TextSwitch {
                text: qsTranslate("tremotesf", "Notify on added torrents")
                onClicked: Settings.notificationOnAddingTorrent = checked
                Component.onCompleted: checked = Settings.notificationOnAddingTorrent
            }

            TextSwitch {
                text: qsTranslate("tremotesf", "Notify on finished torrents")
                onClicked: Settings.notificationOfFinishedTorrents = checked
                Component.onCompleted: checked = Settings.notificationOfFinishedTorrents
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "When connecting to server")
            }

            TextSwitch {
                text: qsTranslate("tremotesf", "Notify on added torrents since last connection to server")
                onClicked: Settings.notificationsOnAddedTorrentsSinceLastConnection = checked
                Component.onCompleted: checked = Settings.notificationsOnAddedTorrentsSinceLastConnection
            }

            TextSwitch {
                text: qsTranslate("tremotesf", "Notify on finished torrents since last connection to server")
                onClicked: Settings.notificationsOnFinishedTorrentsSinceLastConnection = checked
                Component.onCompleted: checked = Settings.notificationsOnFinishedTorrentsSinceLastConnection
            }
        }
    }
}
