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

Item {
    id: item

    property alias label: textField.label
    property alias text: textField.text
    property alias readOnly: textField.readOnly
    property bool selectionButtonEnabled: true

    property bool connectTextFieldWithDialog: true
    property bool fileDialogCanAccept: true
    property string fileDialogErrorString
    property string fileDialogDirectory

    property bool showFiles: true
    property var nameFilters

    signal dialogAccepted(string filePath)

    width: parent.width
    height: Math.max(textField.height, selectionButton.height)

    TextField {
        id: textField

        anchors {
            left: parent.left
            right: selectionButton.left
            verticalCenter: parent.verticalCenter
        }
        errorHighlight: !text
        placeholderText: label

        onTextChanged: {
            if (!showFiles && connectTextFieldWithDialog) {
                fileDialogDirectory = text
            }
        }
    }

    IconButton {
        id: selectionButton

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        enabled: item.enabled && selectionButtonEnabled
        icon.source: "image://theme/icon-m-folder"

        onClicked: {
            var dialog = pageStack.push(fileDialogComponent, {"directory": showFiles ? String() : fileDialogDirectory,
                                                              "showFiles": showFiles,
                                                              "nameFilters": nameFilters,
                                                              "errorString": fileDialogErrorString})
            dialog.accepted.connect(function() {
                if (connectTextFieldWithDialog) {
                    text = dialog.filePath
                }
                dialogAccepted(dialog.filePath)
            })
        }
    }

    Component {
        id: fileDialogComponent

        FileSelectionDialog {
            canAccept: fileDialogCanAccept
            onDirectoryChanged: fileDialogDirectory = directory
        }
    }
}
