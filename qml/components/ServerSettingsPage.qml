/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

Page {
    allowedOrientations: defaultAllowedOrientations

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            DisconnectedHeader { }

            PageHeader {
                title: qsTranslate("tremotesf", "Server Settings")
            }

            SimpleBackgroundItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Downloading", "Noun")
                onClicked: pageStack.push("DownloadingPage.qml")
            }

            SimpleBackgroundItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Seeding", "Noun")
                onClicked: pageStack.push("SeedingPage.qml")
            }

            SimpleBackgroundItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Queue")
                onClicked: pageStack.push("QueuePage.qml")
            }

            SimpleBackgroundItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Speed")
                onClicked: pageStack.push("SpeedPage.qml")
            }

            SimpleBackgroundItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Network")
                onClicked: pageStack.push("NetworkPage.qml")
            }
        }
    }
}
