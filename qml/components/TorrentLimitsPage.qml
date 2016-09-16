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

Page {
    id: torrentLimitsPage

    property var torrent

    allowedOrientations: defaultAllowedOrientations

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
                enabled: torrent
                text: qsTranslate("tremotesf", "Honor global limits")
                onClicked: torrent.honorSessionLimits = checked
                Component.onCompleted: checked = torrent.honorSessionLimits
            }

            TextSwitch {
                id: downloadSpeedLimitSwitch

                enabled: torrent
                text: qsTranslate("tremotesf", "Download")
                onClicked: torrent.downloadSpeedLimited = checked

                Component.onCompleted: checked = torrent.downloadSpeedLimited
            }

            TextField {
                property bool completed: false

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: downloadSpeedLimitSwitch.checked

                enabled: torrent

                label: qsTranslate("tremotesf", "KiB/s")
                placeholderText: label

                inputMethodHints: Qt.ImhDigitsOnly

                validator: IntValidator {
                    bottom: 0
                    top: 4 * 1024 * 1024 - 1
                }

                onTextChanged: {
                    if (completed) {
                        torrent.downloadSpeedLimit = text
                    }
                }

                Component.onCompleted: {
                    text = torrent.downloadSpeedLimit
                    completed = true
                }
            }

            TextSwitch {
                id: uploadSpeedLimitSwitch

                enabled: torrent
                text: qsTranslate("tremotesf", "Upload")
                onClicked: torrent.uploadSpeedLimited = checked

                Component.onCompleted: checked = torrent.uploadSpeedLimited
            }

            TextField {
                property bool completed: false

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: uploadSpeedLimitSwitch.checked

                enabled: torrent

                label: qsTranslate("tremotesf", "KiB/s")
                placeholderText: label

                inputMethodHints: Qt.ImhDigitsOnly

                validator: IntValidator {
                    bottom: 0
                    top: 4 * 1024 * 1024 - 1
                }

                onTextChanged: {
                    if (completed) {
                        torrent.uploadSpeedLimit = text
                    }
                }

                Component.onCompleted: {
                    text = torrent.uploadSpeedLimit
                    completed = true
                }
            }

            ComboBox {
                enabled: torrent
                label: qsTranslate("tremotesf", "Torrent priority")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTranslate("tremotesf", "High")
                        onClicked: torrent.bandwidthPriority = Torrent.HighPriority
                    }

                    MenuItem {
                        text: qsTranslate("tremotesf", "Normal")
                        onClicked: torrent.bandwidthPriority = Torrent.NormalPriority
                    }

                    MenuItem {
                        text: qsTranslate("tremotesf", "Low")
                        onClicked: torrent.bandwidthPriority = Torrent.LowPriority
                    }
                }

                Component.onCompleted: currentIndex = (1 - torrent.bandwidthPriority)
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Seeding")
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

                Component.onCompleted: {
                    switch (torrent.ratioLimitMode) {
                    case Torrent.GlobalRatioLimit:
                        currentIndex = 0
                        break
                    case Torrent.SingleRatioLimit:
                        currentIndex = 2
                        break
                    case Torrent.UnlimitedRatio:
                        currentIndex = 1
                    }
                }
            }

            TextField {
                property bool completed: false

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: ratioLimitComboBox.currentIndex === 2

                enabled: torrent

                label: qsTranslate("tremotesf", "Ratio limit")
                placeholderText: label

                inputMethodHints: Qt.ImhFormattedNumbersOnly

                validator: DoubleValidator {
                    bottom: 0
                    top: 10000
                }

                onTextChanged: {
                    if (completed) {
                        torrent.ratioLimit = Number.fromLocaleString(Qt.locale(), text)
                    }
                }

                Component.onCompleted: {
                    text = torrent.ratioLimit.toLocaleString(Qt.locale(), 'f', 2)
                    completed = true
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

                Component.onCompleted: {
                    switch (torrent.idleSeedingLimitMode) {
                    case Torrent.GlobalIdleSeedingLimit:
                        currentIndex = 0
                        break
                    case Torrent.SingleIdleSeedingLimit:
                        currentIndex = 2
                        break
                    case Torrent.UnlimitedIdleSeeding:
                        currentIndex = 1
                    }
                }
            }

            TextField {
                property bool completed: false

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: idleSeedingComboBox.currentIndex === 2

                enabled: torrent
                label: qsTranslate("tremotesf", "min")
                placeholderText: label
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                    top: 10000
                }

                onTextChanged: {
                    if (completed) {
                        torrent.idleSeedingLimit = text
                    }
                }

                Component.onCompleted: {
                    text = torrent.idleSeedingLimit
                    completed = true
                }
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Peers")
            }

            TextField {
                property bool completed: false

                width: parent.width
                enabled: torrent
                label: qsTranslate("tremotesf", "Maximum peers")
                placeholderText: label
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                    top: 10000
                }

                onTextChanged: {
                    if (completed) {
                        torrent.peersLimit = text
                    }
                }

                Component.onCompleted: {
                    text = torrent.peersLimit
                    completed = true
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
