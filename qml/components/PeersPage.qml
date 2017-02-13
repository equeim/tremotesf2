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
    id: peersPage

    property var torrent: torrentPropertiesPage.torrent

    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: listView

        anchors.fill: parent

        header: Column {
            width: listView.width

            TorrentRemovedHeader {
                torrent: peersPage.torrent
            }

            PageHeader {
                title: qsTranslate("tremotesf", "Peers")
            }
        }

        model: PeersModel {
            id: peersModel
            torrent: peersPage.torrent
        }

        delegate: BackgroundItem {
            property var modelData: model

            height: column.height + Theme.paddingMedium

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
                    text: modelData.address
                    truncationMode: TruncationMode.Fade
                }

                Item {
                    width: parent.width
                    height: childrenRect.height

                    Column {
                        Label {
                            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            text: "\u2193 %1".arg(Utils.formatByteSpeed(modelData.downloadSpeed))
                        }
                        Label {
                            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            text: "\u2191 %1".arg(Utils.formatByteSpeed(modelData.downloadSpeed))
                        }
                    }

                    Column {
                        anchors.right: parent.right

                        Label {
                            anchors.right: parent.right
                            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            text: Utils.formatProgress(modelData.progress)
                        }

                        Label {
                            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            text: modelData.client
                        }
                    }
                }
            }
        }

        BusyIndicator {
            anchors.centerIn: parent
            size: BusyIndicatorSize.Large
            running: !peersModel.loaded
        }

        ViewPlaceholder {
            enabled: peersModel.loaded && (listView.count === 0)
            text: qsTranslate("tremotesf", "No peers")
        }

        VerticalScrollDecorator { }
    }
}
