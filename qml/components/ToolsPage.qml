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

import Sailfish.Silica 1.0
import QtQuick 2.2

Page {
    allowedOrientations: defaultAllowedOrientations

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTranslate("tremotesf", "About")
                onClicked: pageStack.push("AboutPage.qml")
            }
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTranslate("tremotesf", "Tools")
            }

            SimpleBackgroundItem {
                text: qsTranslate("tremotesf", "Settings")
                onClicked: pageStack.push("SettingsPage.qml")
            }

            SimpleBackgroundItem {
                text: qsTranslate("tremotesf", "Accounts")
                onClicked: pageStack.push("AccountsPage.qml")
            }

            Separator {
                width: parent.width
                color: Theme.secondaryColor
            }

            SimpleBackgroundItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Server Settings")
                onClicked: pageStack.push("ServerSettingsPage.qml")
            }
        }
    }
}
