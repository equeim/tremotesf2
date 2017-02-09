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
    id: trackersPage

    property var torrent: torrentPropertiesPage.torrent

    allowedOrientations: defaultAllowedOrientations

    RemorsePopup {
        id: remorsePopup
    }

    SilicaListView {
        id: listView

        property alias selectionModel: selectionModel

        anchors.fill: parent

        header: Column {
            width: listView.width

            TorrentRemovedHeader {
                torrent: trackersPage.torrent
            }

            PageHeader {
                title: qsTranslate("tremotesf", "Trackers")
            }
        }

        model: BaseProxyModel {
            id: proxyModel

            sourceModel: TrackersModel {
                id: trackersModel
                torrent: trackersPage.torrent
            }
            sortRole: TrackersModel.AnnounceRole

            Component.onCompleted: sort()
        }

        delegate: ListItem {
            property var modelData: model
            property bool selected

            function remove() {
                torrent.removeTrackers([modelData.id])
            }

            contentHeight: column.height + Theme.paddingMedium
            highlighted: down || menuOpen || selected

            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTranslate("tremotesf", "Edit...")
                        onClicked: pageStack.push("TrackerEditDialog.qml", {"torrent": torrent,
                                                      "trackerId": modelData.id,
                                                      "announce": modelData.announce})
                    }
                    MenuItem {
                        text: qsTranslate("tremotesf", "Remove")
                        onClicked: remorseAction(qsTranslate("tremotesf", "Removing %1").arg(modelData.announce), remove)
                    }
                }
            }
            showMenuOnPressAndHold: !selectionPanel.openPanel

            onClicked: {
                if (selectionPanel.openPanel) {
                    selectionModel.select(modelData.index)
                } else {
                    pageStack.push("TrackerEditDialog.qml", {"torrent": torrent,
                                       "trackerId": modelData.id,
                                       "announce": modelData.announce})
                }
            }

            onPressAndHold: {
                if (selectionPanel.openPanel) {
                    selectionModel.select(modelData.index)
                }
            }

            Component.onCompleted: selected = selectionModel.isSelected(modelData.index)

            Connections {
                target: selectionModel
                onSelectionChanged: selected = selectionModel.isSelected(modelData.index)
            }

            Column {
                id: column

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeSmall
                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                    text: modelData.announce
                    truncationMode: TruncationMode.Fade
                }

                Item {
                    width: parent.width
                    height: childrenRect.height

                    Label {
                        anchors {
                            left: parent.left
                            right: peersLabel.left
                        }
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        text: modelData.statusString
                        truncationMode: TruncationMode.Fade
                    }

                    Label {
                        id: peersLabel

                        anchors.right: parent.right
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        text: modelData.error ? String()
                                              : qsTranslate("tremotesf", "%n peer(s)", String(), modelData.peers)
                    }
                }

                Label {
                    width: parent.width
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    text: (modelData.nextUpdate < 0) ? String()
                                                     : qsTranslate("tremotesf", "Next update after %1").arg(Utils.formatEta(modelData.nextUpdate))
                    visible: text
                    truncationMode: TruncationMode.Fade
                }
            }
        }

        SelectionModel {
            id: selectionModel
            model: proxyModel
        }

        PullDownMenu {
            MenuItem {
                enabled: torrent
                text: qsTranslate("tremotesf", "Select")
                onClicked: selectionPanel.show()
            }

            MenuItem {
                enabled: torrent
                text: qsTranslate("tremotesf", "Add...")
                onClicked: pageStack.push("TrackerEditDialog.qml", {"torrent": torrent})
            }
        }

        ViewPlaceholder {
            enabled: !listView.count
            text: qsTranslate("tremotesf", "No trackers")
        }

        VerticalScrollDecorator { }
    }

    SelectionPanel {
        id: selectionPanel
        selectionModel: listView.selectionModel
        text: qsTranslate("tremotesf", "%n tracker(s) selected", String(), selectionModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                text: qsTranslate("tremotesf", "Remove")
                onClicked: remorsePopup.execute(qsTranslate("tremotesf", "Removing %n tracker(s)", String(), selectionModel.selectedIndexesCount),
                                                function() {
                                                    torrent.removeTrackers(trackersModel.idsFromIndexes(proxyModel.sourceIndexes(selectionModel.selectedIndexes)))
                                                })
            }
        }

        Connections {
            target: rpc
            onConnectedChanged: {
                if (!rpc.connected) {
                    selectionPanel.hide()
                }
            }
        }
    }
}
