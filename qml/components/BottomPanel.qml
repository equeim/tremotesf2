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

import harbour.tremotesf 1.0

DockedPanel {
    width: parent.width
    height: Theme.itemSizeMedium
    contentHeight: height

    Binding on open {
        id: openBinding
        value: rpc.connected && !selectionPanel.open && !Qt.inputMethod.visible
    }
    onOpenChanged: open = openBinding.value

    opacity: open ? 1 : 0
    Behavior on opacity {
        FadeAnimation { }
    }

    PushUpMenu {
        id: pushUpMenu

        MenuItem {
            text: qsTranslate("tremotesf", "Sort")
            onClicked: pageStack.push(Qt.createComponent("SortTorrentsPage.qml"))
        }

        MenuItem {
            text: qsTranslate("tremotesf", "Filter")
            onClicked: pageStack.push(filterTorrentsPage)

            FilterTorrentsPage {
                id: filterTorrentsPage
            }
        }

        MenuItem {
            text: qsTranslate("tremotesf", "Select")
            onClicked: {
                var showSelectionPanel = function() {
                    selectionPanel.show()
                    pushUpMenu.activeChanged.disconnect(showSelectionPanel)
                }
                pushUpMenu.activeChanged.connect(showSelectionPanel)
            }
        }
    }

    Item {
        implicitWidth: Theme.itemSizeExtraLarge * 3
        width: {
            if (searchButton.x > implicitWidth) {
                return implicitWidth
            } else {
                return searchButton.x
            }
        }
        height: parent.height

        Item {
            width: parent.width / 2
            height: parent.height

            Label {
                id: downArrow

                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter

                font.pixelSize: Theme.fontSizeLarge
                text: "\u2193"
            }

            Label {
                anchors {
                    left: downArrow.right
                    leftMargin: Theme.paddingSmall
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: Theme.fontSizeSmall
                text: Utils.formatByteSpeed(rpc.serverStats.downloadSpeed)
                truncationMode: TruncationMode.Fade
            }
        }

        Item {
            x: parent.width / 2
            width: parent.width / 2
            height: parent.height

            Label {
                id: upArrow

                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter

                font.pixelSize: Theme.fontSizeLarge
                text: "\u2191"
            }

            Label {
                anchors {
                    left: upArrow.right
                    leftMargin: Theme.paddingSmall
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                font.pixelSize: Theme.fontSizeSmall
                text: Utils.formatByteSpeed(rpc.serverStats.uploadSpeed)
                truncationMode: TruncationMode.Fade
            }
        }
    }

    IconButton {
        id: searchButton

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        icon.source: "image://theme/icon-m-search"
        onClicked: searchPanel.show()
    }
}
