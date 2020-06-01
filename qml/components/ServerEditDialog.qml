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

Dialog {
    property var serversModel
    property var modelData
    property bool overwrite: false

    function save() {
        var mountedDirectories = {}
        for (var i = 0, max = mountedDirectoriesModel.count; i < max; ++i) {
            var item = mountedDirectoriesModel.get(i)
            if (item.local && item.remote) {
                mountedDirectories[item.local] = item.remote
            }
        }

        Servers.setServer(modelData ? modelData.name : String(),
                          nameField.text,
                          addressField.text,
                          portField.text,
                          apiPathField.text,

                          proxyTypeComboBox.currentProxyType,
                          proxyHostnameField.text,
                          proxyPortField.text,
                          proxyUserField.text,
                          proxyPasswordField.text,

                          httpsSwitch.checked,
                          selfSignedCertificateSwitch.checked,
                          selfSignedCertificateTextArea.text,
                          clientCertificateSwitch.checked,
                          clientCertificateTextArea.text,
                          authenticationSwitch.checked,
                          usernameField.text,
                          passwordField.text,
                          updateIntervalField.text,
                          backgroundUpdateIntervalField.text,
                          timeoutField.text,
                          mountedDirectories)
        if (serversModel) {
            serversModel.setServer(modelData ? modelData.name : String(),
                                   nameField.text,
                                   addressField.text,
                                   portField.text,
                                   apiPathField.text,

                                   proxyTypeComboBox.currentProxyType,
                                   proxyHostnameField.text,
                                   proxyPortField.text,
                                   proxyUserField.text,
                                   proxyPasswordField.text,

                                   httpsSwitch.checked,
                                   selfSignedCertificateSwitch.checked,
                                   selfSignedCertificateTextArea.text,
                                   clientCertificateSwitch.checked,
                                   clientCertificateTextArea.text,
                                   authenticationSwitch.checked,
                                   usernameField.text,
                                   passwordField.text,
                                   updateIntervalField.text,
                                   backgroundUpdateIntervalField.text,
                                   timeoutField.text,
                                   mountedDirectories)
        }
    }

    acceptDestination: overwrite ? overwriteDialog : pageStack.previousPage()
    acceptDestinationAction: (acceptDestination === pageStack.previousPage()) ? PageStackAction.Pop : PageStackAction.Push

    allowedOrientations: defaultAllowedOrientations
    canAccept: nameField.acceptableInput &&
               addressField.acceptableInput &&
               updateIntervalField.acceptableInput &&
               backgroundUpdateIntervalField.acceptableInput &&
               timeoutField.acceptableInput

    onAccepted: {
        if (!overwrite) {
            save()
        }
    }

    Component {
        id: overwriteDialog

        Dialog {
            acceptDestination: serversPage
            acceptDestinationAction: PageStackAction.Pop
            allowedOrientations: defaultAllowedOrientations

            onAccepted: save()

            Column {
                width: parent.width

                DialogHeader {
                    acceptText: qsTranslate("tremotesf", "Overwrite")
                    title: qsTranslate("tremotesf", "Server already exists")
                }
            }
        }
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        VerticalScrollDecorator { }

        Column {
            id: column
            width: parent.width

            DialogHeader {
                title: modelData ? qsTranslate("tremotesf", "Edit Server") : qsTranslate("tremotesf", "Add Server")
            }

            FormTextField {
                id: nameField

                property string name

                width: parent.width

                label: qsTranslate("tremotesf", "Name")
                placeholderText: label

                text: name
                validator: RegExpValidator {
                    regExp: /^\S.*/
                }

                onTextChanged: {
                    if (serversModel) {
                        overwrite = (text !== name && serversModel.hasServer(text))
                    }
                }

                Component.onCompleted: name = (modelData ? modelData.name : "")
            }

            FormTextField {
                id: addressField

                width: parent.width

                label: qsTranslate("tremotesf", "Address")
                placeholderText: label

                text: modelData ? modelData.address : String()
                inputMethodHints: Qt.ImhNoAutoUppercase
                validator: RegExpValidator {
                    regExp: /^\S+/
                }
            }

            FormTextField {
                id: portField

                width: parent.width

                label: qsTranslate("tremotesf", "Port")
                placeholderText: label

                text: modelData ? modelData.port : "9091"
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 0
                    top: 65535
                }
            }

            FormTextField {
                id: apiPathField

                width: parent.width

                label: qsTranslate("tremotesf", "API path")
                placeholderText: label

                text: modelData ? modelData.apiPath : "/transmission/rpc"
            }

            ComboBox {
                id: proxyTypeComboBox

                property int currentProxyType: currentItem.itemId

                label: qsTranslate("tremotesf", "Proxy type")
                currentItem: menu.itemForId(modelData ? modelData.proxyType : Server.Default)
                menu: ContextMenuWithIds {
                    MenuItemWithId {
                        itemId: Server.Default
                        text: qsTranslate("tremotesf", "Default")
                    }
                    MenuItemWithId {
                        itemId: Server.Http
                        text: qsTranslate("tremotesf", "HTTPS")
                    }
                    MenuItemWithId {
                        itemId: Server.Socks5
                        text: qsTranslate("tremotesf", "SOCKS5")
                    }
                }
            }

            Column {
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: proxyTypeComboBox.currentProxyType !== Server.Default

                FormTextField {
                    id: proxyHostnameField

                    width: parent.width

                    label: qsTranslate("tremotesf", "Address")
                    placeholderText: label

                    text: modelData ? modelData.proxyHostname : String()
                    inputMethodHints: Qt.ImhNoAutoUppercase
                    validator: RegExpValidator {
                        regExp: /^\S+/
                    }
                }

                FormTextField {
                    id: proxyPortField

                    width: parent.width

                    label: qsTranslate("tremotesf", "Port")
                    placeholderText: label

                    text: modelData ? modelData.proxyPort : "0"
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator {
                        bottom: 0
                        top: 65535
                    }
                }

                FormTextField {
                    id: proxyUserField

                    width: parent.width

                    label: qsTranslate("tremotesf", "Username")
                    placeholderText: label

                    text: modelData ? modelData.username : String()
                }

                FormPasswordField {
                    id: proxyPasswordField

                    width: parent.width

                    label: qsTranslate("tremotesf", "Password")
                    placeholderText: label

                    text: modelData ? modelData.password : String()
                }
            }

            TextSwitch {
                id: httpsSwitch
                text: qsTranslate("tremotesf", "HTTPS")
                checked: modelData ? modelData.https : false
            }

            Column {
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: httpsSwitch.checked

                TextSwitch {
                    id: selfSignedCertificateSwitch
                    text: qsTranslate("tremotesf", "Server uses self-signed certificate")
                    checked: modelData ? modelData.selfSignedCertificateEnabled : false
                }

                TextArea {
                    id: selfSignedCertificateTextArea

                    width: parent.width
                    height: Theme.itemSizeHuge * 2
                    visible: selfSignedCertificateSwitch.checked
                    activeFocusOnTab: true

                    label: qsTranslate("tremotesf", "Server's certificate in PEM format")
                    placeholderText: label

                    text: modelData ? modelData.selfSignedCertificate : String()
                }

                TextSwitch {
                    id: clientCertificateSwitch
                    text: qsTranslate("tremotesf", "Use client certificate authentication")
                    checked: modelData ? modelData.clientCertificateEnabled : false
                }

                TextArea {
                    id: clientCertificateTextArea

                    width: parent.width
                    height: Theme.itemSizeHuge * 2
                    visible: clientCertificateSwitch.checked
                    activeFocusOnTab: true

                    label: qsTranslate("tremotesf", "Certificate in PEM format with private key")
                    placeholderText: label

                    text: modelData ? modelData.clientCertificate : String()
                }
            }

            TextSwitch {
                id: authenticationSwitch

                text: qsTranslate("tremotesf", "Authentication")
                checked: modelData ? modelData.authentication : false
            }

            Column {
                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: parent.right
                }
                visible: authenticationSwitch.checked

                FormTextField {
                    id: usernameField

                    width: parent.width

                    label: qsTranslate("tremotesf", "Username")
                    placeholderText: label

                    text: modelData ? modelData.username : String()
                }

                FormPasswordField {
                    id: passwordField

                    width: parent.width

                    label: qsTranslate("tremotesf", "Password")
                    placeholderText: label

                    text: modelData ? modelData.password : String()
                }
            }

            FormTextField {
                id: updateIntervalField

                width: parent.width

                label: qsTranslate("tremotesf", "Update interval, s")
                placeholderText: label

                text: modelData ? modelData.updateInterval : "5"
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 1
                    top: 3600
                }
            }
            
            FormTextField {
                id: backgroundUpdateIntervalField

                width: parent.width

                label: qsTranslate("tremotesf", "Background update interval, s")
                placeholderText: label

                text: modelData ? modelData.backgroundUpdateInterval : "30"
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 1
                    top: 3600
                }
            }

            FormTextField {
                id: timeoutField

                width: parent.width

                label: qsTranslate("tremotesf", "Timeout, s")
                placeholderText: label

                text: modelData ? modelData.timeout : "30"
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 5
                    top: 60
                }
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Mounted directories")
            }

            Repeater {
                id: directoriesRepeater

                model: ListModel {
                    id: mountedDirectoriesModel
                    Component.onCompleted: {
                        if (modelData) {
                            var directories = modelData.mountedDirectories
                            for (var local in directories) {
                                append({"local": local, "remote": directories[local]})
                            }
                        }
                    }
                }

                delegate: Column {
                    property alias localDirectoryTextField: localDirectoryTextField
                    property int lastItem: (index === mountedDirectoriesModel.count - 1)

                    width: column.width

                    Item {
                        width: column.width
                        height: Theme.paddingMedium
                        visible: index > 0

                        Separator {
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width
                            color: Theme.secondaryColor
                        }
                    }

                    FileSelectionItem {
                        id: localDirectoryTextField

                        label: qsTranslate("tremotesf", "Local directory")
                        showFiles: false
                        text: local

                        onTextChanged: local = text
                    }

                    Row {
                        FormTextField {
                            id: remoteDirectoryTextField

                            width: column.width - removeButton.width - Theme.horizontalPageMargin
                            inputMethodHints: Qt.ImhNoAutoUppercase
                            text: remote
                            label: qsTranslate("tremotesf", "Remote directory")
                            placeholderText: label
                            onTextChanged: remote = text
                        }

                        IconButton {
                            id: removeButton
                            anchors.verticalCenter: parent.verticalCenter
                            icon.source: "image://theme/icon-m-remove"
                            onClicked: mountedDirectoriesModel.remove(index)
                        }
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                preferredWidth: Theme.buttonWidthLarge
                text: qsTranslate("tremotesf", "Add")
                onClicked: {
                    mountedDirectoriesModel.append({"local": String(), "remote": String()})
                    flickable.contentY += directoriesRepeater.itemAt(directoriesRepeater.count - 1).height
                }
            }
        }
    }
}
