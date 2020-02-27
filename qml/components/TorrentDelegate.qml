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

ListItem {
    property int torrentId: model.id
    property var torrent

    property string localTorrentFilesPath: torrent ? rpc.localTorrentFilesPath(torrent) : String()
    property bool torrentIsLocal: torrent ? rpc.isTorrentLocalMounted(torrent) && Utils.fileExists(localTorrentFilesPath) : false

    property color primaryTextColor: highlighted ? Theme.highlightColor : Theme.primaryColor
    property color secondaryTextColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

    property bool selected

    highlighted: down || menuOpen || selected

    Connections {
        target: selectionModel
        onSelectionChanged: selected = selectionModel.isSelected(model.index)
    }

    contentHeight: contentItem.height + Theme.paddingMedium
    menu: Component {
        ContextMenu {
            MenuItem {
                id: startMenuItem
                visible: {
                    switch (torrent.status) {
                    case Torrent.Paused:
                    case Torrent.Errored:
                        return true
                    default:
                        return false
                    }
                }
                text: qsTranslate("tremotesf", "Start")
                onClicked: rpc.startTorrents([torrentId])
            }
            MenuItem {
                visible: startMenuItem.visible
                text: qsTranslate("tremotesf", "Start Now")
                onClicked: rpc.startTorrentsNow([torrentId])
            }
            MenuItem {
                visible: !startMenuItem.visible
                text: qsTranslate("tremotesf", "Pause")
                onClicked: rpc.pauseTorrents([torrentId])
            }
            MenuItem {
                text: qsTranslate("tremotesf", "Remove")
                onClicked: pageStack.push("RemoveTorrentsDialog.qml", {"ids": [torrentId]})
            }
            MenuItem {
                text: qsTranslate("tremotesf", "Set Location")
                onClicked: pageStack.push("SetLocationDialog.qml",
                                          {"directory": torrent.downloadDirectory,
                                           "ids": [torrentId]})
            }
            MenuItem {
                visible: torrentIsLocal
                text: qsTranslate("tremotesf", "Open")
                onClicked: Qt.openUrlExternally(localTorrentFilesPath)
            }
            /*MenuItem {
                visible: torrentIsLocal
                text: qsTranslate("tremotesf", "Open Download Directory")
                onClicked: Qt.openUrlExternally(rpc.localTorrentDownloadDirectoryPath(torrent))
            }*/
            MenuItem {
                text: qsTranslate("tremotesf", "Check Local Data")
                onClicked: rpc.checkTorrents([torrentId])
            }
            MenuItem {
                text: qsTranslate("tremotesf", "Reannounce")
                onClicked: rpc.reannounceTorrents([torrentId])
            }
        }
    }
    showMenuOnPressAndHold: !selectionPanel.openPanel

    onClicked: {
        if (selectionPanel.openPanel) {
            selectionModel.select(model.index)
        } else {
            pageStack.push("TorrentPropertiesPage.qml", {"torrentHash": torrent.hashString,
                                                         "torrent": torrent})
        }
    }

    onPressAndHold: {
        if (selectionPanel.openPanel) {
            selectionModel.select(model.index)
        }
    }

    onTorrentIdChanged: torrent = model.torrent

    Component.onCompleted: {
        torrent = model.torrent
        selected = selectionModel.isSelected(model.index)
    }

    Separator {
        width: parent.width
        color: Theme.secondaryColor
    }

    Item {
        id: contentItem

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        height: childrenRect.height

        Item {
            id: nameRow

            width: parent.width
            height: statusImage.height

            Image {
                id: statusImage
                source: {
                    if (!torrent) {
                        return String()
                    }

                    var iconSource

                    switch (torrent.status) {
                    case Torrent.Downloading:
                    case Torrent.Seeding:
                    case Torrent.StalledDownloading:
                    case Torrent.StalledSeeding:
                    case Torrent.QueuedForDownloading:
                    case Torrent.QueuedForSeeding:
                        iconSource = "image://theme/icon-m-play"
                        break;
                    default:
                        iconSource = "image://theme/icon-m-pause"
                    }

                    if (highlighted) {
                        iconSource += "?"
                        iconSource += Theme.highlightColor
                    }

                    return iconSource
                }
                rotation: {
                    if (torrent) {
                        switch (torrent.status) {
                        case Torrent.Downloading:
                        case Torrent.StalledDownloading:
                        case Torrent.QueueForDownloading:
                            return 90
                        case Torrent.Seeding:
                        case Torrent.StalledSeeding:
                        case Torrent.QueuedForSeeding:
                            return -90
                        }
                    }
                    return 0
                }
                sourceSize.width: Theme.iconSizeSmallPlus
                sourceSize.height: Theme.iconSizeSmallPlus
            }

            Label {
                anchors {
                    left: statusImage.right
                    leftMargin: Theme.paddingSmall
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                color: primaryTextColor
                font.pixelSize: Theme.fontSizeSmall
                text: torrent ? Theme.highlightText(torrent.name, searchPanel.text, Theme.highlightColor) : String()
                truncationMode: TruncationMode.Fade
            }
        }

        Label {
            id: progressLabel

            anchors {
                top: nameRow.bottom
                left: parent.left
                right: etaLabel.left
            }
            color: secondaryTextColor
            font.pixelSize: Theme.fontSizeExtraSmall

            text: {
                if (!torrent) {
                    return String()
                }

                if (torrent.percentDone === 1) {
                    //: e.g. 100 MiB, uploaded 200 MiB
                    return qsTranslate("tremotesf", "%1, uploaded %2")
                    .arg(Utils.formatByteSize(torrent.sizeWhenDone))
                    .arg(Utils.formatByteSize(torrent.totalUploaded))
                }

                //: e.g. 100 MiB of 200 MiB (50%)
                return qsTranslate("tremotesf", "%1 of %2 (%3)")
                .arg(Utils.formatByteSize(torrent.completedSize))
                .arg(Utils.formatByteSize(torrent.sizeWhenDone))
                .arg(Utils.formatProgress(torrent.percentDone))
            }
            truncationMode: TruncationMode.Fade
        }

        Label {
            id: etaLabel
            anchors {
                top: nameRow.bottom
                right: parent.right
            }

            color: secondaryTextColor
            font.pixelSize: Theme.fontSizeExtraSmall
            text: torrent ? Utils.formatEta(torrent.eta) : String()
        }

        Item {
            id: progressBar

            anchors.top: progressLabel.bottom
            width: parent.width
            height: Theme.paddingSmall

            Rectangle {
                id: progressRectangle

                anchors.verticalCenter: parent.verticalCenter
                width: {
                    if (torrent) {
                        switch (torrent.status) {
                        case Torrent.Checking:
                        case Torrent.QueuedForChecking:
                            return parent.width * torrent.recheckProgress
                        default:
                            return parent.width * torrent.percentDone
                        }
                    }
                    return 0
                }
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

        Column {
            id: speedColumn
            anchors.top: progressBar.bottom

            Label {
                color: secondaryTextColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: torrent ? "\u2193 %1".arg(Utils.formatByteSpeed(torrent.downloadSpeed)) : String()
            }
            Label {
                color: secondaryTextColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: torrent ? "\u2191 %1".arg(Utils.formatByteSpeed(torrent.uploadSpeed)) : String()
            }
        }

        Label {
            anchors {
                top: progressBar.bottom
                left: speedColumn.right
                leftMargin: Theme.paddingLarge
                right: parent.right
            }

            color: secondaryTextColor
            font.pixelSize: Theme.fontSizeExtraSmall
            horizontalAlignment: Text.AlignRight
            maximumLineCount: 2
            text: {
                if (!torrent) {
                    return String()
                }

                switch (torrent.status) {
                case Torrent.Paused:
                    return qsTranslate("tremotesf", "Paused", "Torrent status")
                case Torrent.Downloading:
                    return qsTranslate("tremotesf", "Downloading from %Ln peers", String(), torrent.seeders)
                case Torrent.StalledDownloading:
                    return qsTranslate("tremotesf", "Downloading", "Torrent status")
                case Torrent.Seeding:
                    return qsTranslate("tremotesf", "Seeding to %Ln peers", String(), torrent.leechers)
                case Torrent.StalledSeeding:
                    return qsTranslate("tremotesf", "Seeding", "Torrent status")
                case Torrent.QueuedForDownloading:
                case Torrent.QueuedForSeeding:
                    return qsTranslate("tremotesf", "Queued", "Torrent status")
                case Torrent.Checking:
                    return qsTranslate("tremotesf", "Checking (%L1)").arg(Utils.formatProgress(torrent.recheckProgress))
                case Torrent.QueuedForChecking:
                    return qsTranslate("tremotesf", "Queued for checking")
                case Torrent.Errored:
                    return torrent.errorString
                default:
                    return String()
                }
            }
            truncationMode: TruncationMode.Elide
            wrapMode: Text.WordWrap
        }
    }
}
