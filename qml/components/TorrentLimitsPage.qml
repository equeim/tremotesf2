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

        priorityComboBox.currentIndex = (1 - torrent.bandwidthPriority)

        switch (torrent.ratioLimitMode) {
        case Torrent.GlobalRatioLimit:
            ratioLimitComboBox.currentIndex = 0
            break
        case Torrent.SingleRatioLimit:
            ratioLimitComboBox.currentIndex = 2
            break
        case Torrent.UnlimitedRatio:
            ratioLimitComboBox.currentIndex = 1
        }
        ratioLimitField.text = torrent.ratioLimit.toLocaleString(Qt.locale(), 'f', 2)

        switch (torrent.idleSeedingLimitMode) {
        case Torrent.GlobalIdleSeedingLimit:
            idleSeedingComboBox.currentIndex = 0
            break
        case Torrent.SingleIdleSeedingLimit:
            idleSeedingComboBox.currentIndex = 2
            break
        case Torrent.UnlimitedIdleSeeding:
            idleSeedingComboBox.currentIndex = 1
        }
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

            TextField {
                id: downloadSpeedLimitField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: downloadSpeedLimitSwitch.checked

                enabled: torrent
                opacity: enabled ? 1.0 : 0.4

                label: qsTranslate("tremotesf", "KiB/s")
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

            TextField {
                id: uploadSpeedLimitField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: uploadSpeedLimitSwitch.checked

                enabled: torrent
                opacity: enabled ? 1.0 : 0.4

                label: qsTranslate("tremotesf", "KiB/s")
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

                enabled: torrent
                label: qsTranslate("tremotesf", "Torrent priority")
                menu: ContextMenu {
                    MenuItem {
                        //: Priority
                        text: qsTranslate("tremotesf", "High")
                        onClicked: torrent.bandwidthPriority = Torrent.HighPriority
                    }

                    MenuItem {
                        //: Priority
                        text: qsTranslate("tremotesf", "Normal")
                        onClicked: torrent.bandwidthPriority = Torrent.NormalPriority
                    }

                    MenuItem {
                        //: Priority
                        text: qsTranslate("tremotesf", "Low")
                        onClicked: torrent.bandwidthPriority = Torrent.LowPriority
                    }
                }
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Seeding", "Noun")
            }

            ComboBox {
                id: ratioLimitComboBox

                enabled: torrent
                label: qsTranslate("tremotesf", "Ratio limit mode")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTranslate("tremotesf", "Use global settings")
                        onClicked: torrent.ratioLimitMode = Torrent.GlobalRatioLimit
                    }

                    MenuItem {
                        text: qsTranslate("tremotesf", "Seed regardless of ratio")
                        onClicked: torrent.ratioLimitMode = Torrent.UnlimitedRatio
                    }

                    MenuItem {
                        text: qsTranslate("tremotesf", "Stop seeding at ratio:")
                        onClicked: torrent.ratioLimitMode = Torrent.SingleRatioLimit
                    }
                }
            }

            TextField {
                id: ratioLimitField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: ratioLimitComboBox.currentIndex === 2

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

                enabled: torrent
                label: qsTranslate("tremotesf", "Idle seeding mode")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTranslate("tremotesf", "Use global settings")
                        onClicked: torrent.idleSeedingLimitMode = Torrent.GlobalIdleSeedingLimit
                    }

                    MenuItem {
                        text: qsTranslate("tremotesf", "Seed regardless of activity")
                        onClicked: torrent.idleSeedingLimitMode = Torrent.UnlimitedIdleSeeding
                    }

                    MenuItem {
                        text: qsTranslate("tremotesf", "Stop seeding if idle for:")
                        onClicked: torrent.idleSeedingLimitMode = Torrent.SingleIdleSeedingLimit
                    }
                }
            }

            TextField {
                id: idleSeedingField

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: idleSeedingComboBox.currentIndex === 2

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

            TextField {
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
