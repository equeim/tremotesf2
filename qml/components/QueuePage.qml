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

import Sailfish.Silica 1.0
import QtQuick 2.2

Page {
    allowedOrientations: defaultAllowedOrientations

    property bool loaded: false

    function loadSettings() {
        downloadQueueSwitch.checked = rpc.serverSettings.downloadQueueEnabled
        downloadQueueTextField.text = rpc.serverSettings.downloadQueueSize
        seedQueueSwitch.checked = rpc.serverSettings.seedQueueEnabled
        seedQueueTextField.text = rpc.serverSettings.seedQueueSize
        idleQueueSwitch.checked = rpc.serverSettings.idleQueueLimited
        idleQueueTextField.text = rpc.serverSettings.idleQueueLimit

        loaded = true
    }

    function setEnabled(enabled) {
        downloadQueueSwitch.enabled = enabled
        downloadQueueTextField.enabled = enabled
        seedQueueSwitch.enabled = enabled
        seedQueueTextField.enabled = enabled
        idleQueueSwitch.enabled = enabled
        idleQueueTextField.enable = enabled
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
                title: qsTranslate("tremotesf", "Queue")
            }

            TextSwitch {
                id: downloadQueueSwitch
                text: qsTranslate("tremotesf", "Maximum active downloads")
                onClicked: rpc.serverSettings.downloadQueueEnabled = checked
            }

            TextField {
                id: downloadQueueTextField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: downloadQueueSwitch.checked
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.downloadQueueSize = text
                    }
                }
            }

            TextSwitch {
                id: seedQueueSwitch
                text: qsTranslate("tremotesf", "Maximum active uploads")
                onClicked: rpc.serverSettings.seedQueueEnabled = checked
            }

            TextField {
                id: seedQueueTextField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: seedQueueSwitch.checked
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.seedQueueSize = text
                    }
                }
            }

            TextSwitch {
                id: idleQueueSwitch
                text: qsTranslate("tremotesf", "Ignore queue position if idle for")
                onClicked: rpc.serverSettings.idleQueueLimited = checked
            }

            TextField {
                id: idleQueueTextField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: idleQueueSwitch.checked
                label: qsTranslate("tremotesf", "min")
                placeholderText: label
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.idleQueueLimit = text
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
