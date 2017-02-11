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

ListItem {
    property var torrent

    property color primaryTextColor: highlighted ? Theme.highlightColor : Theme.primaryColor
    property color secondaryTextColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

    property bool selected

    highlighted: down || menuOpen || selected

    Connections {
        target: selectionModel
        onSelectionChanged: selected = selectionModel.isSelected(model.index)
    }

    contentHeight: column.height + Theme.paddingMedium
    menu: Component {
        ContextMenu {
            MenuItem {
                text: qsTranslate("tremotesf", "Properties")
                onClicked: pageStack.push("TorrentPropertiesPage.qml", {"torrentHash": torrent.hashString,
                                                                        "torrent": torrent})
            }
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
                onClicked: rpc.startTorrents([torrent.id])
            }
            MenuItem {
                visible: startMenuItem.visible
                text: qsTranslate("tremotesf", "Start Now")
                onClicked: rpc.startTorrentsNow([torrent.id])
            }
            MenuItem {
                visible: !startMenuItem.visible
                text: qsTranslate("tremotesf", "Pause")
                onClicked: rpc.pauseTorrents([torrent.id])
            }
            MenuItem {
                text: qsTranslate("tremotesf", "Remove")
                onClicked: pageStack.push("RemoveTorrentsDialog.qml", {"ids": [torrent.id]})
            }
            MenuItem {
                text: qsTranslate("tremotesf", "Check Local Data")
                onClicked: rpc.checkTorrents([torrent.id])
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

    Component.onCompleted: {
        torrent = torrentsModel.torrentAtRow(torrentsProxyModel.sourceRow(model.index))
        selected = selectionModel.isSelected(model.index)
    }

    Separator {
        width: parent.width
        color: Theme.secondaryColor
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

        Item {
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
                    verticalCenter: statusImage.verticalCenter
                }
                color: primaryTextColor
                font.pixelSize: Theme.fontSizeSmall
                text: torrent ? Theme.highlightText(torrent.name, searchPanel.text, Theme.highlightColor) : String()
                truncationMode: TruncationMode.Fade
            }
        }

        Item {
            width: parent.width
            height: childrenRect.height

            Label {
                anchors {
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
                        return qsTranslate("tremotesf", "%1, uploaded %2")
                        .arg(Utils.formatByteSize(torrent.sizeWhenDone))
                        .arg(Utils.formatByteSize(torrent.totalUploaded))
                    }

                    return qsTranslate("tremotesf", "%1 of %2 (%3)")
                    .arg(Utils.formatByteSize(torrent.completedSize))
                    .arg(Utils.formatByteSize(torrent.sizeWhenDone))
                    .arg(Utils.formatProgress(torrent.percentDone))
                }
                truncationMode: TruncationMode.Fade
            }

            Label {
                id: etaLabel
                anchors.right: parent.right
                color: secondaryTextColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: torrent ? Utils.formatEta(torrent.eta) : String()
            }
        }

        Item {
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

        Item {
            width: parent.width
            height: childrenRect.height

            Column {
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

            Column {
                anchors {
                    left: parent.horizontalCenter
                    right: parent.right
                }

                Label {
                    width: parent.width
                    color: secondaryTextColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: implicitWidth > width ? Text.AlignLeft : Text.AlignRight
                    truncationMode: TruncationMode.Fade
                    text: {
                        if (torrent) {
                            if (torrent.status === Torrent.Checking) {
                                return qsTranslate("tremotesf", "Checking (%1)").arg(Utils.formatProgress(torrent.recheckProgress))
                            }
                            return torrent.statusString
                        }
                        return String()
                    }
                }

                Label {
                    anchors.right: parent.right
                    color: secondaryTextColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    text: {
                        if (torrent) {
                            switch (torrent.status) {
                            case Torrent.Downloading:
                                return qsTranslate("tremotesf", "from %n peer(s)", String(), torrent.seeders)
                            case Torrent.Seeding:
                                return qsTranslate("tremotesf", "to %n peer(s)", String(), torrent.leechers)
                            }
                        }
                        return String()
                    }
                }
            }
        }
    }
}
