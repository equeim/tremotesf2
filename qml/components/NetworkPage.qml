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

import harbour.tremotesf 1.0

Page {
    allowedOrientations: defaultAllowedOrientations

    property bool loaded: false

    function loadSettings() {
        peerPortTextField.text = rpc.serverSettings.peerPort.toLocaleString()
        randomPortSwitch.checked = rpc.serverSettings.randomPortEnabled
        portFormardingSwitch.checked = rpc.serverSettings.portForwardingEnabled
        encryptionComboBox.update()
        utpSwitch.checked = rpc.serverSettings.utpEnabled
        pexSwitch.checked = rpc.serverSettings.pexEnabled
        dhtSwitch.checked = rpc.serverSettings.dhtEnabled
        lpdSwitch.checked = rpc.serverSettings.lpdEnabled
        peersPerTorrentTextField.text = rpc.serverSettings.maximumPeerPerTorrent
        peersGloballyTextField.text = rpc.serverSettings.maximumPeersGlobally

        loaded = true
    }

    function setEnabled(enabled) {
        peerPortTextField.enabled = enabled
        randomPortSwitch.enabled = enabled
        portFormardingSwitch.enabled = enabled
        encryptionComboBox.enabled = enabled
        utpSwitch.enabled = enabled
        pexSwitch.enabled = enabled
        dhtSwitch.enabled = enabled
        lpdSwitch.enabled = enabled
        peersPerTorrentTextField.enabled = enabled
        peersGloballyTextField.enabled = enabled
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
                title: qsTranslate("tremotesf", "Network")
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Connection")
            }

            TextField {
                id: peerPortTextField

                width: parent.width
                label: qsTranslate("tremotesf", "Peer port")
                placeholderText: label
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                    top: 65535
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.peerPort = text
                    }
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: peersPerTorrentTextField.forceActiveFocus()
            }

            TextSwitch {
                id: randomPortSwitch
                text: qsTranslate("tremotesf", "Random port on Transmission start")
                onClicked: rpc.serverSettings.randomPortEnabled = checked
            }

            TextSwitch {
                id: portFormardingSwitch
                text: qsTranslate("tremotesf", "Enable port forwarding")
                onClicked: rpc.serverSettings.portForwardingEnabled = checked
            }

            ComboBox {
                id: encryptionComboBox

                function update() {
                    console.log(rpc.serverSettings.encryptionMode)
                    currentItem = menu.itemForId(rpc.serverSettings.encryptionMode)
                }

                label: qsTranslate("tremotesf", "Encryption")
                menu: ContextMenuWithIds {
                    MenuItemWithId {
                        itemId: ServerSettings.AllowedEncryption
                        text: qsTranslate("tremotesf", "Allow")
                        onClicked: rpc.serverSettings.encryptionMode = itemId
                    }

                    MenuItemWithId {
                        itemId: ServerSettings.PreferredEncryption
                        text: qsTranslate("tremotesf", "Prefer")
                        onClicked: rpc.serverSettings.encryptionMode = itemId
                    }

                    MenuItemWithId {
                        itemId: ServerSettings.RequiredEncryption
                        text: qsTranslate("tremotesf", "Require")
                        onClicked: rpc.serverSettings.encryptionMode = itemId
                    }
                }
            }

            TextSwitch {
                id: utpSwitch
                text: qsTranslate("tremotesf", "Enable uTP")
                onClicked: rpc.serverSettings.utpEnabled = checked
            }

            TextSwitch {
                id: pexSwitch
                text: qsTranslate("tremotesf", "Enable PEX")
                onClicked: rpc.serverSettings.pexEnabled = checked
            }

            TextSwitch {
                id: dhtSwitch
                text: qsTranslate("tremotesf", "Enable DHT")
                onClicked: rpc.serverSettings.dhtEnabled = checked
            }

            TextSwitch {
                id: lpdSwitch
                text: qsTranslate("tremotesf", "Enable LPD")
                onClicked: rpc.serverSettings.lpdEnabled = checked
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Peer Limits")
            }

            TextField {
                id: peersPerTorrentTextField

                width: parent.width
                label: qsTranslate("tremotesf", "Maximum peers per torrent")
                placeholderText: label
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.maximumPeerPerTorrent = text
                    }
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: peersGloballyTextField.forceActiveFocus()
            }

            TextField {
                id: peersGloballyTextField

                width: parent.width
                label: qsTranslate("tremotesf", "Maximum peers globally")
                placeholderText: label
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.maximumPeersGlobally = text
                    }
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
            }
        }

        VerticalScrollDecorator { }
    }
}
