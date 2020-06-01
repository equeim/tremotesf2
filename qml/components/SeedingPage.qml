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
        ratioLimitSwitch.checked = rpc.serverSettings.ratioLimited
        ratioLimitTextField.text = rpc.serverSettings.ratioLimit.toLocaleString(Qt.locale(), 'f', 2)
        idleSeedingLimitSwitch.checked = rpc.serverSettings.idleSeedingLimited
        idleSeedingLimitTextField.text = rpc.serverSettings.idleSeedingLimit

        loaded = true
    }

    function setEnabled(enabled) {
        ratioLimitSwitch.enabled = enabled
        ratioLimitTextField.enabled = enabled
        idleSeedingLimitSwitch.enabled = enabled
        idleSeedingLimitTextField.enabled = enabled
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
                title: qsTranslate("tremotesf", "Seeding", "Noun")
            }

            TextSwitch {
                id: ratioLimitSwitch
                text: qsTranslate("tremotesf", "Stop seeding at ratio")
                onClicked: rpc.serverSettings.ratioLimited = checked
            }

            FormTextField {
                id: ratioLimitTextField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: ratioLimitSwitch.checked
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                validator: DoubleValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.ratioLimit = Number.fromLocaleString(Qt.locale(), text)
                    }
                }
            }

            TextSwitch {
                id: idleSeedingLimitSwitch
                text: qsTranslate("tremotesf", "Stop seeding if idle for")
                onClicked: rpc.serverSettings.idleSeedingLimited = checked
            }

            FormTextField {
                id: idleSeedingLimitTextField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: idleSeedingLimitSwitch.checked
                //: Minutes
                label: qsTranslate("tremotesf", "min")
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.idleSeedingLimit = text
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
