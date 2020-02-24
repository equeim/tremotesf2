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

Dialog {
    allowedOrientations: defaultAllowedOrientations
    canAccept: !textField.errorHighlight

    onAccepted: filesModel.renameFile(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)),
                                      textField.text)

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: qsTranslate("tremotesf", "Rename")
            }

            TextField {
                id: textField

                width: parent.width
                errorHighlight: !text
                label: qsTranslate("tremotesf", "File name")
                placeholderText: label
                text: modelData.name

                EnterKey.enabled: text
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: accept()
            }
        }
    }
}
