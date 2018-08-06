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
    id: addTorrentFileDialog

    property string filePath

    allowedOrientations: defaultAllowedOrientations
    canAccept: filesModel.loaded && downloadDirectoryItem.text

    onAccepted: rpc.addTorrentFile(parser.fileData,
                                   downloadDirectoryItem.text,
                                   filesModel.wantedFiles,
                                   filesModel.unwantedFiles,
                                   filesModel.highPriorityFiles,
                                   filesModel.normalPriorityFiles,
                                   filesModel.lowPriorityFiles,
                                   1 - priorityComboBox.currentIndex,
                                   startSwitch.checked)

    TorrentFileParser {
        id: parser
        filePath: addTorrentFileDialog.filePath
        onDone: {
            if (error === TorrentFileParser.NoError) {
                filesModel.load(parser)
            }
        }
    }

    LocalTorrentFilesModel {
        id: filesModel
    }

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: {
            if (!filePath) {
                return false
            }

            if (parser.error === TorrentFileParser.NoError) {
                return !parser.loaded || !filesModel.loaded
            }

            return false
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        ViewPlaceholder {
            enabled: parser.error !== TorrentFileParser.NoError
            text: parser.errorString
        }

        Column {
            id: column

            width: parent.width
            visible: filesModel.loaded

            DialogHeader {
                title: qsTranslate("tremotesf", "Add Torrent File")
            }

            TextField {
                width: parent.width
                label: qsTranslate("tremotesf", "Torrent file")
                text: filePath
                readOnly: true
            }

            FileSelectionItem {
                id: downloadDirectoryItem

                property bool mounted: !rpc.local && Servers.currentServerHasMountedDirectories
                property bool settingTextFromDialog: false

                label: qsTranslate("tremotesf", "Download directory")
                selectionButtonEnabled: rpc.local || mounted
                showFiles: false

                connectTextFieldWithDialog: rpc.local
                fileDialogCanAccept: mounted ? Servers.isUnderCurrentServerMountedDirectory(fileDialogDirectory) : true
                fileDialogErrorString: qsTranslate("tremotesf", "Selected directory is not inside mounted directory")

                Component.onCompleted: {
                    text = rpc.serverSettings.downloadDirectory
                    if (mounted && !fileDialogDirectory) {
                        fileDialogDirectory = Servers.firstLocalDirectory
                    }
                }

                onTextChanged: {
                    var path = text.trim()
                    if (rpc.serverSettings.canShowFreeSpaceForPath) {
                        rpc.getFreeSpaceForPath(path)
                    } else {
                        if (path === rpc.serverSettings.downloadDirectory) {
                            rpc.getDownloadDirFreeSpace()
                        } else {
                            freeSpaceLabel.visible = false
                            freeSpaceLabel.text = String()
                        }
                    }

                    if (mounted && !settingTextFromDialog) {
                        var directory = Servers.fromRemoteToLocalDirectory(path)
                        if (directory) {
                            fileDialogDirectory = directory
                        }
                    }
                }

                onDialogAccepted: {
                    if (mounted) {
                        settingTextFromDialog = true
                        text = Servers.fromLocalToRemoteDirectory(filePath)
                        settingTextFromDialog = false
                    }
                }
            }

            Label {
                id: freeSpaceLabel

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                }

                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor

                Connections {
                    target: rpc
                    onGotDownloadDirFreeSpace: {
                        if (downloadDirectoryItem.text.trim() === rpc.serverSettings.downloadDirectory) {
                            freeSpaceLabel.text = qsTranslate("tremotesf", "Free space: %1").arg(Utils.formatByteSize(bytes))
                            freeSpaceLabel.visible = true
                        }
                    }
                    onGotFreeSpaceForPath: {
                        if (path === downloadDirectoryItem.text.trim()) {
                            if (success) {
                                freeSpaceLabel.text = qsTranslate("tremotesf", "Free space: %1").arg(Utils.formatByteSize(bytes))
                            } else {
                                freeSpaceLabel.text = qsTranslate("tremotesf", "Error getting free space")
                            }
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: filesButton.height + Theme.paddingLarge

                Button {
                    id: filesButton

                    anchors.centerIn: parent
                    width: Theme.buttonWidthLarge
                    text: qsTranslate("tremotesf", "Files")

                    onClicked: pageStack.push("LocalTorrentFilesPage.qml", {"filesModel": filesModel})
                }
            }

            ComboBox {
                id: priorityComboBox

                label: qsTranslate("tremotesf", "Torrent priority")
                menu: ContextMenu {
                    MenuItem {
                        //: Priority
                        text: qsTranslate("tremotesf", "High")
                    }
                    MenuItem {
                        //: Priority
                        text: qsTranslate("tremotesf", "Normal")
                    }
                    MenuItem {
                        //: Priority
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
