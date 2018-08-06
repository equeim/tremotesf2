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
    allowedOrientations: defaultAllowedOrientations

    property bool loaded: false

    function loadSettings() {
        downloadSpeedLimitSwitch.checked = rpc.serverSettings.downloadSpeedLimited
        downloadSpeedLimitTextField.text = rpc.serverSettings.downloadSpeedLimit
        uploadSpeedLimitSwitch.checked = rpc.serverSettings.uploadSpeedLimited
        uploadSpeedLimitTextField.text = rpc.serverSettings.uploadSpeedLimit
        alternativeLimitsSwitch.checked = rpc.serverSettings.alternativeSpeedLimitsEnabled
        alternativeDownloadSpeedLimitTextField.text = rpc.serverSettings.alternativeDownloadSpeedLimit
        alternativeUploadSpeedLimitTextField.text = rpc.serverSettings.alternativeUploadSpeedLimit
        scheduleSwitch.checked = rpc.serverSettings.alternativeSpeedLimitsScheduled
        beginTimeButton.time = rpc.serverSettings.alternativeSpeedLimitsBeginTime
        endTimeButton.time = rpc.serverSettings.alternativeSpeedLimitsEndTime

        var days = rpc.serverSettings.alternativeSpeedLimitsDays
        for (var i = 0, max = daysComboBox.menu.children.length; i < max; i++) {
            var menuItem = daysComboBox.menu.children[i]
            if (menuItem.days === days) {
                daysComboBox.currentIndex = i
                break
            }
        }

        loaded = true
    }

    function setEnabled(enabled) {
        downloadSpeedLimitSwitch.enabled = enabled
        downloadSpeedLimitTextField.enabled = enabled
        uploadSpeedLimitSwitch.enabled = enabled
        uploadSpeedLimitTextField.enabled = enabled
        alternativeLimitsSwitch.enabled = enabled
        alternativeDownloadSpeedLimitTextField.enabled = enabled
        alternativeUploadSpeedLimitTextField.enabled = enabled
        scheduleSwitch.enabled = enabled
        beginTimeButton.enabled = enabled
        endTimeButton.enabled = enabled
        daysComboBox.enabled = enabled
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
                title: qsTranslate("tremotesf", "Speed")
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Limits")
            }

            TextSwitch {
                id: downloadSpeedLimitSwitch
                text: qsTranslate("tremotesf", "Download", "Noun")
                onClicked: rpc.serverSettings.downloadSpeedLimited = checked
            }

            TextField {
                id: downloadSpeedLimitTextField
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: downloadSpeedLimitSwitch.checked
                label: qsTranslate("tremotesf", "KiB/s")
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.downloadSpeedLimit = text
                    }
                }

                EnterKey.iconSource: uploadSpeedLimitSwitch.checked || alternativeLimitsSwitch.checked ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    if (uploadSpeedLimitSwitch.checked) {
                        uploadSpeedLimitTextField.forceActiveFocus()
                    } else if (alternativeLimitsSwitch.checked) {
                        alternativeDownloadSpeedLimitTextField.forceActiveFocus()
                    }
                }
            }

            TextSwitch {
                id: uploadSpeedLimitSwitch
                //: Noun
                text: qsTranslate("tremotesf", "Upload")
                onClicked: rpc.serverSettings.uploadSpeedLimited = checked
            }

            TextField {
                id: uploadSpeedLimitTextField
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: uploadSpeedLimitSwitch.checked
                label: qsTranslate("tremotesf", "KiB/s")
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.uploadSpeedLimit = text
                    }
                }

                EnterKey.iconSource: alternativeLimitsSwitch.checked ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-close"
                EnterKey.onClicked: {
                    if (alternativeLimitsSwitch.checked) {
                        alternativeDownloadSpeedLimitTextField.forceActiveFocus()
                    }
                }
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Alternative Limits")
            }

            TextSwitch {
                id: alternativeLimitsSwitch
                text: qsTranslate("tremotesf", "Enable")
                onClicked: rpc.serverSettings.alternativeSpeedLimitsEnabled = checked
            }

            TextField {
                id: alternativeDownloadSpeedLimitTextField
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: alternativeLimitsSwitch.checked
                label: qsTranslate("tremotesf", "Download, KiB/s")
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.alternativeDownloadSpeedLimit = text
                    }
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: alternativeUploadSpeedLimitTextField.forceActiveFocus()
            }

            TextField {
                id: alternativeUploadSpeedLimitTextField
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: alternativeLimitsSwitch.checked
                label: qsTranslate("tremotesf", "Upload, KiB/s")
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                }

                onTextChanged: {
                    if (loaded) {
                        rpc.serverSettings.alternativeUploadSpeedLimit = text
                    }
                }

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
            }

            TextSwitch {
                id: scheduleSwitch
                text: qsTranslate("tremotesf", "Scheduled")
                onClicked: rpc.serverSettings.alternativeSpeedLimitsScheduled = checked
            }

            Row {
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: scheduleSwitch.checked

                AlternativeLimitsTimeButton {
                    id: beginTimeButton
                    width: parent.width / 2
                    //: e.g. inside "From 1:00 AM to 5:00 AM"
                    label: qsTranslate("tremotesf", "From")
                    onTimeChanged: {
                        if (loaded) {
                            rpc.serverSettings.alternativeSpeedLimitsBeginTime = time
                        }
                    }
                }

                AlternativeLimitsTimeButton {
                    id: endTimeButton
                    width: parent.width / 2
                    //: e.g. inside "From 1:00 AM to 5:00 AM"
                    label: qsTranslate("tremotesf", "to")
                    onTimeChanged: {
                        if (loaded) {
                            rpc.serverSettings.alternativeSpeedLimitsEndTime = time
                        }
                    }
                }
            }

            ComboBox {
                id: daysComboBox

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: scheduleSwitch.checked

                label: qsTranslate("tremotesf", "Days")
                menu: ContextMenu {
                    DaysComboBoxMenuItem {
                        days: ServerSettings.All
                        text: qsTranslate("tremotesf", "Every day")
                    }

                    DaysComboBoxMenuItem {
                        days: ServerSettings.Weekdays
                        text: qsTranslate("tremotesf", "Weekdays")
                    }

                    DaysComboBoxMenuItem {
                        days: ServerSettings.Weekends
                        text: qsTranslate("tremotesf", "Weekends")
                    }

                    DaysComboBoxMenuItem { }
                    DaysComboBoxMenuItem { }
                    DaysComboBoxMenuItem { }
                    DaysComboBoxMenuItem { }
                    DaysComboBoxMenuItem { }
                    DaysComboBoxMenuItem { }
                    DaysComboBoxMenuItem { }
                }

                Component.onCompleted: {
                    var nextDay = function(day) {
                        if (day === Qt.Sunday) {
                            return Qt.Monday
                        }
                        return (day + 1)
                    }

                    var first = Qt.locale().firstDayOfWeek

                    for (var day = first, i = 3; i < 10; day = nextDay(day), i++) {
                        var menuItem = menu.children[i]

                        menuItem.text = Qt.locale().dayName(day)

                        switch (day) {
                        case Qt.Monday:
                            menuItem.days = ServerSettings.Monday
                            break
                        case Qt.Tuesday:
                            menuItem.days = ServerSettings.Tuesday
                            break
                        case Qt.Wednesday:
                            menuItem.days = ServerSettings.Wednesday
                            break
                        case Qt.Thursday:
                            menuItem.days = ServerSettings.Thursday
                            break
                        case Qt.Friday:
                            menuItem.days = ServerSettings.Friday
                            break
                        case Qt.Saturday:
                            menuItem.days = ServerSettings.Saturday
                            break
                        case Qt.Sunday:
                            menuItem.days = ServerSettings.Sunday
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
