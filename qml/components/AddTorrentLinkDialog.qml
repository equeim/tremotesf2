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

import QtQuick 2.2
import Sailfish.Silica 1.0

import harbour.tremotesf 1.0

Dialog {
    property alias url: torrentLinkTextField.text

    allowedOrientations: defaultAllowedOrientations
    canAccept: torrentLinkTextField.text && downloadDirectoryItem.text

    onAccepted: rpc.addTorrentLink(torrentLinkTextField.text,
                                   downloadDirectoryItem.text,
                                   1 - priorityComboBox.currentIndex,
                                   startSwitch.checked)

    TorrentFileParser {
        id: parser
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: qsTranslate("tremotesf", "Add Torrent Link")
            }

            TextField {
                id: torrentLinkTextField

                width: parent.width
                errorHighlight: !text
                label: qsTranslate("tremotesf", "Torrent Link")
                placeholderText: label
            }

            FileSelectionItem {
                id: downloadDirectoryItem

                label: qsTranslate("tremotesf", "Download directory")
                selectionButtonEnabled: rpc.local
                showFiles: false
                Component.onCompleted: text = rpc.serverSettings.downloadDirectory
            }

            ComboBox {
                id: priorityComboBox

                label: qsTranslate("tremotesf", "Torrent priority")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTranslate("tremotesf", "High")
                    }
                    MenuItem {
                        text: qsTranslate("tremotesf", "Normal")
                    }
                    MenuItem {
                        text: qsTranslate("tremotesf", "Low")
                    }
                }
                currentIndex: 1
            }

            TextSwitch {
                id: startSwitch
                text: qsTranslate("tremotesf", "Start downloading after adding")
                Component.onCompleted: checked = rpc.serverSettings.startAddedTorrents
            }
        }

        VerticalScrollDecorator { }
    }
}
