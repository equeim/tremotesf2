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

Page {
    allowedOrientations: defaultAllowedOrientations

    property bool loaded: false

    function loadSettings() {
        downloadDirectoryItem.text = rpc.serverSettings.downloadDirectory
        downloadDirectoryItem.selectionButtonEnabled = rpc.local
        startAddedTorrentsSwitch.checked = rpc.serverSettings.startAddedTorrents
        //trashTorrentFilesSwitch.checked = rpc.serverSettings.trashTorrentFiles
        renameIncompleteFilesSwitch.checked = rpc.serverSettings.renameIncompleteFiles
        incompleteDirectorySwitch.checked = rpc.serverSettings.incompleteDirectoryEnabled
        incompleteDirectoryItem.text = rpc.serverSettings.incompleteDirectory
        incompleteDirectoryItem.selectionButtonEnabled = rpc.local

        loaded = true
    }

    function setEnabled(enabled) {
        downloadDirectoryItem.enabled = enabled
        startAddedTorrentsSwitch.enabled = enabled
        //trashTorrentFilesSwitch.enabled = enabled
        renameIncompleteFilesSwitch.enabled = enabled
        incompleteDirectorySwitch.enabled = enabled
        incompleteDirectoryItem.enabled = enabled
    }

    Component.onCompleted: loadSettings()

    Connections {
        target: rpc
        onConnectedChanged: {
            if (rpc.connected) {
                loaded = false
                loadSettings()
                setEnabled(true)
            } else {
                setEnabled(false)
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            DisconnectedHeader { }

            PageHeader {
                title: qsTranslate("tremotesf", "Downloading", "Noun")
            }

            FileSelectionItem {
                id: downloadDirectoryItem
                label: qsTranslate("tremotesf", "Download directory")
                selectionButtonEnabled: rpc.local
                showFiles: false

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.downloadDirectory = text
                    }
                }

                enterKeyIconSource: incompleteDirectorySwitch.checked ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-close"
                onEnterKeyClicked:{
                    if (incompleteDirectorySwitch.checked) {
                        incompleteDirectoryItem.forceActiveFocus()
                    }
                }
            }

            TextSwitch {
                id: startAddedTorrentsSwitch
                text: qsTranslate("tremotesf", "Start added torrents")
                onClicked: rpc.serverSettings.startAddedTorrents = checked
            }

            /*TextSwitch {
                id: trashTorrentFilesSwitch
                text: qsTranslate("tremotesf", "Trash .torrent files")
                onClicked: rpc.serverSettings.trashTorrentFiles = checked
            }*/

            TextSwitch {
                id: renameIncompleteFilesSwitch
                text: qsTranslate("tremotesf", "Append \".part\" to names of incomplete files")
                onClicked: rpc.serverSettings.renameIncompleteFiles = checked
            }

            TextSwitch {
                id: incompleteDirectorySwitch
                text: qsTranslate("tremotesf", "Directory for incomplete files")
                onClicked: rpc.serverSettings.incompleteDirectoryEnabled = checked
            }

            FileSelectionItem {
                id: incompleteDirectoryItem

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: incompleteDirectorySwitch.checked
                selectionButtonEnabled: rpc.local
                showFiles: false

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.incompleteDirectory = text
                    }
                }

                enterKeyIconSource: "image://theme/icon-m-enter-close"
            }
        }

        VerticalScrollDecorator { }
    }
}
