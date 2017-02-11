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
import QtQml.Models 2.1
import Sailfish.Silica 1.0

import harbour.tremotesf 1.0

Page {
    id: localTorrentFilesPage

    property var filesModel

    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: listView

        property alias selectionModel: selectionModel

        anchors {
            fill: parent
            bottomMargin: selectionPanel.visibleSize
        }
        clip: true

        header: Column {
            width: listView.width

            onHeightChanged: {
                if (listView.contentY < 0) {
                    listView.positionViewAtBeginning()
                }
            }

            PageHeader {
                title: qsTranslate("tremotesf", "Files")
            }

            BackgroundItem {
                id: parentDirectoryItem

                visible: delegateModel.rootIndex !== delegateModel.parentModelIndex()
                onClicked: {
                    if (!selectionPanel.openPanel) {
                        delegateModel.rootIndex = delegateModel.parentModelIndex()
                    }
                }

                Image {
                    id: parentDirectoryIcon
                    anchors {
                        left: parent.left
                        leftMargin: Theme.horizontalPageMargin
                        verticalCenter: parent.verticalCenter
                    }
                    source: {
                        var iconSource = "image://theme/icon-m-folder"
                        if (parentDirectoryItem.highlighted) {
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

                Label {
                    anchors {
                        left: parentDirectoryIcon.right
                        leftMargin: Theme.paddingMedium
                        verticalCenter: parent.verticalCenter
                    }
                    color: parentDirectoryItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: ".."
                }
            }
        }

        model: DelegateModel {
            id: delegateModel

            model: TorrentFilesProxyModel {
                id: filesProxyModel
                sourceModel: filesModel
                sortRole: LocalTorrentFilesModel.NameRole
                Component.onCompleted: sort()
            }

            delegate: ListItem {
                id: delegate

                property var modelData: model
                property bool selected

                highlighted: down || menuOpen || selected

                menu: Component {
                    ContextMenu {
                        MenuItem {
                            visible: modelData.wantedState !== TorrentFilesModelEntryEnums.Wanted
                            text: qsTranslate("tremotesf", "Download", "File menu item")
                            onClicked: filesModel.setFileWanted(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), true)
                        }

                        MenuItem {
                            visible: modelData.wantedState !== TorrentFilesModelEntryEnums.Unwanted
                            text: qsTranslate("tremotesf", "Not Download")
                            onClicked: filesModel.setFileWanted(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), false)
                        }

                        MenuLabel {
                            text: qsTranslate("tremotesf", "Priority")
                        }

                        MenuItem {
                            font.bold: modelData.priority === TorrentFilesModelEntryEnums.HighPriority
                            text: qsTranslate("tremotesf", "High")
                            onClicked: {
                                if (modelData.priority !== TorrentFilesModelEntryEnums.HighPriority) {
                                    filesModel.setFilePriority(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), TorrentFilesModelEntryEnums.HighPriority)
                                }
                            }
                        }

                        MenuItem {
                            font.bold: modelData.priority === TorrentFilesModelEntryEnums.NormalPriority
                            text: qsTranslate("tremotesf", "Normal")
                            onClicked: {
                                if (modelData.priority !== TorrentFilesModelEntryEnums.NormalPriority) {
                                    filesModel.setFilePriority(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), TorrentFilesModelEntryEnums.NormalPriority)
                                }
                            }
                        }

                        MenuItem {
                            font.bold: modelData.priority === TorrentFilesModelEntryEnums.LowPriority
                            text: qsTranslate("tremotesf", "Low")
                            onClicked: {
                                if (modelData.priority !== TorrentFilesModelEntryEnums.LowPriority) {
                                    filesModel.setFilePriority(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), TorrentFilesModelEntryEnums.LowPriority)
                                }
                            }
                        }

                        MenuItem {
                            visible: modelData.priority === TorrentFilesModelEntryEnums.MixedPriority
                            font.bold: true
                            text: qsTranslate("tremotesf", "Mixed")
                        }
                    }
                }
                showMenuOnPressAndHold: !selectionPanel.openPanel

                onClicked: {
                    if (selectionPanel.openPanel) {
                        selectionModel.select(delegateModel.modelIndex(modelData.index))
                    } else if (modelData.isDirectory) {
                        delegateModel.rootIndex = delegateModel.modelIndex(modelData.index)
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

                    Label {
                        width: parent.width
                        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                        text: Utils.formatByteSize(modelData.size)
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
                    checked: modelData.wantedState !== TorrentFilesModelEntryEnums.Unwanted
                    opacity: (modelData.wantedState === TorrentFilesModelEntryEnums.MixedWanted) ? 0.6 : 1

                    onClicked: filesModel.setFileWanted(filesProxyModel.sourceIndex(delegateModel.modelIndex(modelData.index)), !checked)
                }
            }
        }

        SelectionModel {
            id: selectionModel
            model: filesProxyModel
        }

        PullDownMenu {
            MenuItem {
                text: qsTranslate("tremotesf", "Select")
                onClicked: selectionPanel.show()
            }
        }

        VerticalScrollDecorator { }
    }

    SelectionPanel {
        id: selectionPanel
        selectionModel: listView.selectionModel
        parentIndex: delegateModel.rootIndex
        text: qsTranslate("tremotesf", "%n file(s) selected", String(), selectionModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Download", "File menu item")
                onClicked: {
                    filesModel.setFilesWanted(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), true)
                    selectionPanel.hide()
                }
            }

            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Not Download")
                onClicked: {
                    filesModel.setFilesWanted(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), false)
                    selectionPanel.hide()
                }
            }

            MenuLabel {
                text: qsTranslate("tremotesf", "Priority")
            }

            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "High")
                onClicked: {
                    filesModel.setFilesPriority(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), TorrentFilesModelEntryEnums.HighPriority)
                    selectionPanel.hide()
                }
            }

            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Normal")
                onClicked: {
                    filesModel.setFilesPriority(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), TorrentFilesModelEntryEnums.NormalPriority)
                    selectionPanel.hide()
                }
            }

            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Low")
                onClicked: {
                    filesModel.setFilesPriority(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), TorrentFilesModelEntryEnums.LowPriority)
                    selectionPanel.hide()
                }
            }
        }
    }
}
