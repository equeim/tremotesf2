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

DockedPanel {
    property var selectionModel
    property var parentIndex
    property bool openPanel: false
    property alias text: label.text

    function show() {
        openPanel = true
    }

    function hide() {
        openPanel = false
        selectionModel.clear()
    }

    width: parent.width
    height: column.height + Theme.paddingLarge
    contentHeight: height

    Binding on open {
        id: openBinding
        value: openPanel && !Qt.inputMethod.visible
    }
    onOpenChanged: open = openBinding.value

    opacity: open ? 1 : 0
    Behavior on opacity {
        FadeAnimation { }
    }

    Column {
        id: column

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            id: label
            width: parent.width
            horizontalAlignment: implicitWidth > width ? Text.AlignRight : Text.AlignHCenter
            truncationMode: TruncationMode.Fade
        }

        Item {
            width: parent.width
            height: Math.max(buttons.height, closeButton.height)

            Item {
                id: buttons

                anchors {
                    left: parent.left
                    right: closeButton.left
                    verticalCenter: parent.verticalCenter
                }
                height: childrenRect.height

                Button {
                    anchors {
                        left: parent.left
                        right: parent.horizontalCenter
                        rightMargin: Theme.paddingSmall
                    }
                    width: parent.buttonWidth
                    text: qsTranslate("tremotesf", "All")
                    onClicked: selectionModel.selectAll(parentIndex)
                }

                Button {
                    anchors {
                        left: parent.horizontalCenter
                        leftMargin: Theme.paddingSmall
                        right: parent.right
                    }
                    text: qsTranslate("tremotesf", "None")
                    onClicked: selectionModel.clear()
                }
            }

            IconButton {
                id: closeButton

                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                icon.source: "image://theme/icon-m-close"
                onClicked: hide()
            }
        }
    }
}
