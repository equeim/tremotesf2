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
    id: torrentPropertiesPage

    property string torrentHash
    property var torrent
    property bool torrentIsLocal: torrent ? rpc.isTorrentLocalMounted(torrent.downloadDirectory) : false

    function updateTorrent() {
        var newTorrent = rpc.torrentByHash(torrentHash)
        if (newTorrent !== torrent) {
            torrent = newTorrent
        }
    }

    function update() {
        if (!torrent) {
            return
        }

        pageHeader.title = torrent.name

        //: e.g. 100 MiB of 200 MiB (50%)
        progressLabel.value = qsTranslate("tremotesf", "%1 of %2 (%3)")
        .arg(Utils.formatByteSize(torrent.completedSize))
        .arg(Utils.formatByteSize(torrent.sizeWhenDone))
        .arg(Utils.formatProgress(torrent.percentDone))

        downloadedLabel.value = Utils.formatByteSize(torrent.totalDownloaded)
        uploadedLabel.value = Utils.formatByteSize(torrent.totalUploaded)
        ratioLabel.value = Utils.formatRatio(torrent.ratio)
        downloadSpeedLabel.value = Utils.formatByteSpeed(torrent.downloadSpeed)
        uploadSpeedLabel.value = Utils.formatByteSpeed(torrent.uploadSpeed)
        etaLabel.value = Utils.formatEta(torrent.eta)
        seedersLabel.value = torrent.seeders
        leechersLabel.value = torrent.leechers
        activityLabel.value = torrent.activityDate.toLocaleString(Qt.locale(), Locale.ShortFormat)
        totalSizeLabel.value = Utils.formatByteSize(torrent.totalSize)
        locationLabel.value = torrent.downloadDirectory
        creatorLabel.value = torrent.creator
        creationDateLabel.value = torrent.creationDate.toLocaleString(Qt.locale(), Locale.ShortFormat)
        commentLabel.value = torrent.comment
    }

    allowedOrientations: defaultAllowedOrientations

    onTorrentChanged: update()
    Component.onCompleted: update()

    Connections {
        target: rpc
        onTorrentsUpdated: updateTorrent()
    }

    Connections {
        target: torrent
        onUpdated: update()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                enabled: torrent
                text: qsTranslate("tremotesf", "Reannounce")
                onClicked: rpc.reannounceTorrents([torrent.id])
            }
            MenuItem {
                enabled: torrent
                text: qsTranslate("tremotesf", "Check Local Data")
                onClicked: rpc.checkTorrents([torrent.id])
            }
            /*MenuItem {
                visible: torrentIsLocal
                text: qsTranslate("tremotesf", "Open Download Directory")
                onClicked: Qt.openUrlExternally(rpc.localTorrentDownloadDirectoryPath(torrent))
            }*/
            MenuItem {
                visible: torrentIsLocal
                text: qsTranslate("tremotesf", "Open")
                onClicked: Qt.openUrlExternally(rpc.localTorrentFilesPath(torrent))
            }
            MenuItem {
                enabled: torrent
                text: qsTranslate("tremotesf", "Set Location")
                onClicked: pageStack.push("SetLocationDialog.qml",
                                          {"directory": torrent.downloadDirectory,
                                           "ids": [torrent.id]})
            }
            MenuItem {
                enabled: torrent
                text: qsTranslate("tremotesf", "Remove")
                onClicked: pageStack.push("RemoveTorrentsDialog.qml", {"ids": [torrent.id]})
            }
            MenuItem {
                enabled: torrent
                visible: !startMenuItem.visible
                text: qsTranslate("tremotesf", "Pause")
                onClicked: rpc.pauseTorrents([torrent.id])
            }
            MenuItem {
                enabled: torrent
                visible: startMenuItem.visible
                text: qsTranslate("tremotesf", "Start Now")
                onClicked: rpc.startTorrentsNow([torrent.id])
            }
            MenuItem {
                id: startMenuItem

                enabled: torrent
                visible: {
                    if (torrent) {
                        switch (torrent.status) {
                        case Torrent.Paused:
                        case Torrent.Errored:
                            return true
                        }
                    }
                    return false
                }
                text: qsTranslate("tremotesf", "Start")
                onClicked: rpc.startTorrents([torrent.id])
            }
        }

        Column {
            id: column
            width: parent.width

            TorrentRemovedHeader {
                torrent: torrentPropertiesPage.torrent
            }

            PageHeader {
                id: pageHeader
            }

            Grid {
                id: buttonsGrid

                anchors.horizontalCenter: parent.horizontalCenter

                property int implicitButtonWidth: Theme.itemSizeExtraLarge * 2
                property int buttonWidth: (parent.width - spacing * (columns - 1)) / columns

                columns: Math.round(parent.width / implicitButtonWidth)
                spacing: Theme.paddingMedium

                GridButton {
                    enabled: torrent
                    text: qsTranslate("tremotesf", "Files")
                    onClicked: pageStack.push(Qt.createComponent("TorrentFilesPage.qml"))
                }

                GridButton {
                    enabled: torrent
                    text: qsTranslate("tremotesf", "Trackers")
                    onClicked: pageStack.push(Qt.createComponent("TrackersPage.qml"))
                }

                GridButton {
                    enabled: torrent
                    text: qsTranslate("tremotesf", "Peers")
                    onClicked: pageStack.push(Qt.createComponent("PeersPage.qml"))
                }

                GridButton {
                    enabled: torrent
                    text: qsTranslate("tremotesf", "Limits")
                    onClicked: pageStack.push(Qt.createComponent("TorrentLimitsPage.qml"))
                }
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Activity")
            }

            DetailItem {
                id: progressLabel
                //: Torrent's completed size
                label: qsTranslate("tremotesf", "Completed")
            }

            DetailItem {
                id: downloadedLabel
                //: Torrent's downloaded size
                label: qsTranslate("tremotesf", "Downloaded")
            }

            DetailItem {
                id: uploadedLabel
                //: Torrent's uploaded size
                label: qsTranslate("tremotesf", "Uploaded")
            }

            DetailItem {
                id: ratioLabel
                label: qsTranslate("tremotesf", "Ratio")
            }

            DetailItem {
                id: downloadSpeedLabel
                label: qsTranslate("tremotesf", "Download speed")
            }

            DetailItem {
                id: uploadSpeedLabel
                label: qsTranslate("tremotesf", "Upload speed")
            }

            DetailItem {
                id: etaLabel
                label: qsTranslate("tremotesf", "ETA")
            }

            DetailItem {
                id: seedersLabel
                label: qsTranslate("tremotesf", "Seeders")
            }

            DetailItem {
                id: leechersLabel
                label: qsTranslate("tremotesf", "Leechers")
            }

            DetailItem {
                id: activityLabel
                label: qsTranslate("tremotesf", "Last activity")
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Information")
            }

            DetailItem {
                id: totalSizeLabel
                label: qsTranslate("tremotesf", "Total size")
            }

            InteractiveDetailItem {
                id: locationLabel
                label: qsTranslate("tremotesf", "Location")
            }

            InteractiveDetailItem {
                label: qsTranslate("tremotesf", "Hash")
                value: torrentHash
            }

            DetailItem {
                id: creatorLabel
                label: qsTranslate("tremotesf", "Created by")
            }

            DetailItem {
                id: creationDateLabel
                label: qsTranslate("tremotesf", "Created on")
            }

            InteractiveDetailItem {
                id: commentLabel
                label: qsTranslate("tremotesf", "Comment")
            }
        }

        VerticalScrollDecorator { }
    }
}

