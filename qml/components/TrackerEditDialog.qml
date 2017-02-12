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

import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {
    property var torrent
    property int trackerId: -1
    property alias announce: textField.text

    allowedOrientations: defaultAllowedOrientations
    canAccept: !textField.errorHighlight

    onAccepted: {
        if (trackerId === -1) {
            torrent.addTracker(announce)
        } else {
            torrent.setTracker(trackerId, announce)
        }
    }

    Column {
        width: parent.width

        DialogHeader {
            title: (trackerId === -1) ? qsTranslate("tremotesf", "Add Tracker")
                                      : qsTranslate("tremotesf", "Edit Tracker")
        }

        TextField {
            id: textField

            width: parent.width
            errorHighlight: !text
            label: qsTranslate("tremotesf", "Tracker announce URL")
            placeholderText: label

            EnterKey.enabled: text
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: accept()
        }
    }
}
