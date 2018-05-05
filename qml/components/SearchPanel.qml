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

import QtQuick 2.2
import Sailfish.Silica 1.0

//import harbour.tremotesf 1.0

DockedPanel {
    property alias text: searchField.text
    property bool openPanel: false

    function show() {
        openPanel = true
        searchField.forceActiveFocus()
    }

    function hide() {
        openPanel = false
        searchField.focus = false
        text = String()
    }

    Binding on open {
        value: openPanel
    }
    onOpenChanged: open = openPanel

    width: parent.width
    height: Theme.itemSizeMedium
    dock: Dock.Top

    opacity: open ? 1 : 0
    Behavior on opacity {
        FadeAnimation { }
    }

    Connections {
        target: rpc
        onConnectedChanged: {
            if (!rpc.connected) {
                hide()
            }
        }
    }

    SearchField {
        id: searchField

        anchors {
            left: parent.left
            right: closeButton.left
            rightMargin: -Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        onTextChanged: torrentsProxyModel.searchString = text
    }

    IconButton {
        id: closeButton

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        icon.source: "image://theme/icon-m-close"

        onClicked: hide()
    }
}
