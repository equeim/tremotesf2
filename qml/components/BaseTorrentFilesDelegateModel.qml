/*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

import QtQuick 2.6
import QtQml.Models 2.3

import Sailfish.Silica 1.0

import harbour.tremotesf 1.0


DelegateModel {
    id: delegateModel

    property bool addingTorrent

    delegate: ListItem {
        id: delegate

        property var modelData: model
        property bool selected

        highlighted: down || menuOpen || selected

        menu: Component {
            ContextMenu {
                MenuItem {
                    property string filePath: !addingTorrent && torrentPropertiesPage.torrentIsLocal
                                              ? filesModel.localFilePath(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)))
                                              : String()

                    id: openMenuItem
                    visible: !addingTorrent &&
                             modelData.wantedState !== TorrentFilesModelEntry.Unwanted &&
                             torrentPropertiesPage.torrentIsLocal &&
                             Utils.fileExists(filePath)
                    text: qsTranslate("tremotesf", "Open")
                    onClicked: Qt.openUrlExternally(filePath)
                }

                Separator {
                    visible: openMenuItem.visible
                    width: parent.width
                    color: Theme.secondaryColor
                }

                MenuItem {
                    visible: modelData.wantedState !== TorrentFilesModelEntry.Wanted
                    text: qsTranslate("tremotesf", "Download", "File menu item, verb")
                    onClicked: filesModel.setFileWanted(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), true)
                }

                MenuItem {
                    visible: modelData.wantedState !== TorrentFilesModelEntry.Unwanted
                    text: qsTranslate("tremotesf", "Not Download")
                    onClicked: filesModel.setFileWanted(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), false)
                }

                Separator {
                    width: parent.width
                    color: Theme.secondaryColor
                }

                MenuLabel {
                    text: qsTranslate("tremotesf", "Priority")
                }

                MenuItem {
                    font.bold: modelData.priority === TorrentFilesModelEntry.HighPriority
                    //: Priority
                    text: qsTranslate("tremotesf", "High")
                    onClicked: {
                        if (modelData.priority !== TorrentFilesModelEntry.HighPriority) {
                            filesModel.setFilePriority(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), TorrentFilesModelEntry.HighPriority)
                        }
                    }
                }

                MenuItem {
                    font.bold: modelData.priority === TorrentFilesModelEntry.NormalPriority
                    //: Priority
                    text: qsTranslate("tremotesf", "Normal")
                    onClicked: {
                        if (modelData.priority !== TorrentFilesModelEntry.NormalPriority) {
                            filesModel.setFilePriority(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), TorrentFilesModelEntry.NormalPriority)
                        }
                    }
                }

                MenuItem {
                    font.bold: modelData.priority === TorrentFilesModelEntry.LowPriority
                    //: Priority
                    text: qsTranslate("tremotesf", "Low")
                    onClicked: {
                        if (modelData.priority !== TorrentFilesModelEntry.LowPriority) {
                            filesModel.setFilePriority(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), TorrentFilesModelEntry.LowPriority)
                        }
                    }
                }

                MenuItem {
                    visible: modelData.priority === TorrentFilesModelEntry.MixedPriority
                    font.bold: true
                    //: Priority
                    text: qsTranslate("tremotesf", "Mixed")
                }

                Separator {
                    width: parent.width
                    color: Theme.secondaryColor
                }

                MenuItem {
                    text: qsTranslate("tremotesf", "Rename")
                    onClicked: pageStack.push(renameDialogComponent)
                    Component.onCompleted: visible = rpc.serverSettings.canRenameFiles
                }
            }
        }
        showMenuOnPressAndHold: !selectionPanel.openPanel

        onClicked: {
            if (selectionPanel.openPanel) {
                selectionModel.select(delegateModel.modelIndex(modelData.index))
            } else if (modelData.isDirectory) {
                delegateModel.rootIndex = delegateModel.modelIndex(modelData.index)
            } else if (!addingTorrent && torrentPropertiesPage.torrentIsLocal && modelData.wantedState !== TorrentFilesModelEntry.Unwanted) {
                Qt.openUrlExternally(filesModel.localFilePath(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index))))
            }
        }

        onPressAndHold: {
            if (selectionPanel.openPanel) {
                selectionModel.select(delegateModel.modelIndex(modelData.index))
            }
        }

        Component.onCompleted: selected = selectionModel.isSelected(delegateModel.modelIndex(modelData.index))

        Connections {
            target: selectionModel
            onSelectionChanged: selected = selectionModel.isSelected(delegateModel.modelIndex(modelData.index))
        }

        Component {
            id: renameDialogComponent
            FileRenameDialog { }
        }

        Image {
            id: icon
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            source: {
                var iconSource
                if (modelData.isDirectory) {
                    iconSource = "image://theme/icon-m-folder"
                } else {
                    iconSource = "image://theme/icon-m-other"
                }

                if (highlighted) {
                    iconSource += "?"
                    iconSource += Theme.highlightColor
                }
                return iconSource
            }
            sourceSize {
                width: Theme.iconSizeSmallPlus
                height: Theme.iconSizeSmallPlus
            }
        }

        Column {
            id: column

            anchors {
                left: icon.right
                leftMargin: Theme.paddingMedium
                right: wantedSwitch.left
                verticalCenter: parent.verticalCenter
            }

            Label {
                width: parent.width
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: modelData.name
                truncationMode: TruncationMode.Fade
            }

            Loader {
                active: !addingTorrent

                sourceComponent: Item {
                    width: column.width
                    height: Theme.paddingSmall

                    Rectangle {
                        id: progressRectangle

                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width * modelData.progress
                        height: parent.height / 2

                        color: Theme.highlightColor
                        opacity: 0.6
                    }

                    Rectangle {
                        anchors {
                            left: progressRectangle.right
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        height: progressRectangle.height

                        color: Theme.highlightBackgroundColor
                        opacity: Theme.highlightBackgroundOpacity
                    }
                }
            }

            Label {
                width: parent.width
                color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall

                //: e.g. 100 MiB of 200 MiB (50%)
                text: addingTorrent ? Utils.formatByteSize(modelData.size)
                : qsTranslate("tremotesf", "%1 of %2 (%3)")
                .arg(Utils.formatByteSize(modelData.completedSize))
                .arg(Utils.formatByteSize(modelData.size))
                .arg(Utils.formatProgress(modelData.progress))

                truncationMode: TruncationMode.Fade
            }
        }

        Switch {
            id: wantedSwitch

            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            highlighted: down || delegate.highlighted

            enabled: !selectionPanel.openPanel

            automaticCheck: false
            checked: modelData.wantedState !== TorrentFilesModelEntry.Unwanted
            opacity: (modelData.wantedState === TorrentFilesModelEntry.MixedWanted) ? 0.6 : 1

            onClicked: filesModel.setFileWanted(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), !checked)
        }
    }
}
