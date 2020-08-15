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

import harbour.tremotesf 1.0

Page {
    id: torrentLimitsPage

    property var torrent: torrentPropertiesPage.torrent
    property bool updating: false

    function update() {
        if (!torrent) {
            return
        }

        updating = true

        globalLimitsSwitch.checked = torrent.honorSessionLimits

        downloadSpeedLimitSwitch.checked = torrent.downloadSpeedLimited
        downloadSpeedLimitField.text = torrent.downloadSpeedLimit

        uploadSpeedLimitSwitch.checked = torrent.uploadSpeedLimited
        uploadSpeedLimitField.text = torrent.uploadSpeedLimit

        priorityComboBox.update()

        ratioLimitComboBox.update()
        ratioLimitField.text = torrent.ratioLimit.toLocaleString(Qt.locale(), 'f', 2)

        idleSeedingComboBox.update()
        idleSeedingField.text = torrent.idleSeedingLimit

        peersLimitField.text = torrent.peersLimit

        updating = false
    }

    allowedOrientations: defaultAllowedOrientations

    onTorrentChanged: update()
    Component.onCompleted: update()

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            TorrentRemovedHeader {
                torrent: torrentLimitsPage.torrent
            }

            PageHeader {
                title: qsTranslate("tremotesf", "Limits")
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Speed")
            }

            TextSwitch {
                id: globalLimitsSwitch
                enabled: torrent
                text: qsTranslate("tremotesf", "Honor global limits")
                onClicked: torrent.honorSessionLimits = checked
            }

            TextSwitch {
                id: downloadSpeedLimitSwitch
                enabled: torrent
                text: qsTranslate("tremotesf", "Download", "Noun")
                onClicked: torrent.downloadSpeedLimited = checked
            }

            FormTextField {
                id: downloadSpeedLimitField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: downloadSpeedLimitSwitch.checked

                enabled: torrent
                opacity: enabled ? 1.0 : 0.4

                label: qsTranslate("tremotesf", "kB/s")
                placeholderText: label

                inputMethodHints: Qt.ImhDigitsOnly

                validator: IntValidator {
                    bottom: 0
                    top: 4 * 1024 * 1024 - 1
                }

                onTextChanged: {
                    if (!updating) {
                        torrent.downloadSpeedLimit = text
                    }
                }
            }

            TextSwitch {
                id: uploadSpeedLimitSwitch
                enabled: torrent
                text: qsTranslate("tremotesf", "Upload")
                onClicked: torrent.uploadSpeedLimited = checked
            }

            FormTextField {
                id: uploadSpeedLimitField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: uploadSpeedLimitSwitch.checked

                enabled: torrent
                opacity: enabled ? 1.0 : 0.4

                label: qsTranslate("tremotesf", "kB/s")
                placeholderText: label

                inputMethodHints: Qt.ImhDigitsOnly

                validator: IntValidator {
                    bottom: 0
                    top: 4 * 1024 * 1024 - 1
                }

                onTextChanged: {
                    if (!updating) {
                        torrent.uploadSpeedLimit = text
                    }
                }
            }

            ComboBox {
                id: priorityComboBox

                function update() {
                    currentItem = menu.itemForId(torrent.bandwidthPriority)
                }

                enabled: torrent
                label: qsTranslate("tremotesf", "Torrent priority")
                menu: ContextMenuWithIds {
                    MenuItemWithId {
                        itemId: Torrent.HighPriority
                        //: Priority
                        text: qsTranslate("tremotesf", "High")
                        onClicked: torrent.bandwidthPriority = itemId
                    }

                    MenuItemWithId {
                        itemId: Torrent.NormalPriority
                        //: Priority
                        text: qsTranslate("tremotesf", "Normal")
                        onClicked: torrent.bandwidthPriority = itemId
                    }

                    MenuItemWithId {
                        itemId: Torrent.LowPriority
                        //: Priority
                        text: qsTranslate("tremotesf", "Low")
                        onClicked: torrent.bandwidthPriority = itemId
                    }
                }
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Seeding", "Noun")
            }

            ComboBox {
                id: ratioLimitComboBox

                property int currentRatioLimitMode: currentItem.itemId

                function update() {
                    currentItem = menu.itemForId(torrent.ratioLimitMode)
                }

                enabled: torrent
                label: qsTranslate("tremotesf", "Ratio limit mode")
                menu: ContextMenuWithIds {
                    MenuItemWithId {
                        itemId: Torrent.GlobalRatioLimit
                        text: qsTranslate("tremotesf", "Use global settings")
                        onClicked: torrent.ratioLimitMode = itemId
                    }

                    MenuItemWithId {
                        itemId: Torrent.UnlimitedRatio
                        text: qsTranslate("tremotesf", "Seed regardless of ratio")
                        onClicked: torrent.ratioLimitMode = itemId
                    }

                    MenuItemWithId {
                        itemId: Torrent.SingleRatioLimit
                        text: qsTranslate("tremotesf", "Stop seeding at ratio:")
                        onClicked: torrent.ratioLimitMode = itemId
                    }
                }
            }

            FormTextField {
                id: ratioLimitField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: ratioLimitComboBox.currentRatioLimitMode === Torrent.SingleRatioLimit

                enabled: torrent
                opacity: enabled ? 1.0 : 0.4

                label: qsTranslate("tremotesf", "Ratio limit")
                placeholderText: label

                inputMethodHints: Qt.ImhFormattedNumbersOnly

                validator: DoubleValidator {
                    bottom: 0
                    top: 10000
                }

                onTextChanged: {
                    if (!updating) {
                        torrent.ratioLimit = Number.fromLocaleString(Qt.locale(), text)
                    }
                }
            }

            ComboBox {
                id: idleSeedingComboBox

                property int currentIdleSeedingLimitMode: currentItem.itemId

                function update() {
                    currentItem = menu.itemForId(torrent.idleSeedingLimitMode)
                }

                enabled: torrent
                label: qsTranslate("tremotesf", "Idle seeding mode")
                menu: ContextMenuWithIds {
                    MenuItemWithId {
                        itemId: Torrent.GlobalIdleSeedingLimit
                        text: qsTranslate("tremotesf", "Use global settings")
                        onClicked: torrent.idleSeedingLimitMode = itemId
                    }

                    MenuItemWithId {
                        itemId: Torrent.UnlimitedIdleSeeding
                        text: qsTranslate("tremotesf", "Seed regardless of activity")
                        onClicked: torrent.idleSeedingLimitMode = itemId
                    }

                    MenuItemWithId {
                        itemId: Torrent.SingleIdleSeedingLimit
                        text: qsTranslate("tremotesf", "Stop seeding if idle for:")
                        onClicked: torrent.idleSeedingLimitMode = itemId
                    }
                }
            }

            FormTextField {
                id: idleSeedingField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: idleSeedingComboBox.currentIdleSeedingLimitMode === Torrent.SingleIdleSeedingLimit

                enabled: torrent
                opacity: enabled ? 1.0 : 0.4

                //: Minutes
                label: qsTranslate("tremotesf", "min")
                placeholderText: label

                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                    top: 10000
                }

                onTextChanged: {
                    if (!updating) {
                        torrent.idleSeedingLimit = text
                    }
                }
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Peers")
            }

            FormTextField {
                id: peersLimitField

                width: parent.width

                enabled: torrent
                opacity: enabled ? 1.0 : 0.4

                label: qsTranslate("tremotesf", "Maximum peers")
                placeholderText: label

                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                    top: 10000
                }

                onTextChanged: {
                    if (!updating) {
                        torrent.peersLimit = text
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
