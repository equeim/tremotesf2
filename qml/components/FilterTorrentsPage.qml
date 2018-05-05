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

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTranslate("tremotesf", "Filter")
            }

            TextSwitch {
                id: statusSwitch

                property var statusFilter

                checked: true
                text: qsTranslate("tremotesf", "Status")

                onCheckedChanged: {
                    if (checked) {
                        torrentsProxyModel.statusFilter = statusFilter
                    } else {
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.All
                    }
                }

                Component.onCompleted: statusFilter = torrentsProxyModel.statusFilter
            }

            Column {
                width: parent.width
                visible: statusSwitch.checked

                StatusFilterStats {
                    id: statusFilterStats
                    rpc: rootWindow.rpc
                }

                FilterTorrentsListItem {
                    current: torrentsProxyModel.statusFilter === TorrentsProxyModel.All
                    //: All torrents
                    text: qsTranslate("tremotesf", "All")
                    torrents: rpc.torrentsCount

                    onClicked: {
                        statusSwitch.statusFilter = TorrentsProxyModel.All
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.All
                    }
                }

                FilterTorrentsListItem {
                    current: torrentsProxyModel.statusFilter === TorrentsProxyModel.Active
                    text: qsTranslate("tremotesf", "Active", "Active torrents")
                    torrents: statusFilterStats.activeTorrents

                    onClicked: {
                        statusSwitch.statusFilter = TorrentsProxyModel.Active
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.Active
                    }
                }

                FilterTorrentsListItem {
                    current: torrentsProxyModel.statusFilter === TorrentsProxyModel.Downloading
                    text: qsTranslate("tremotesf", "Downloading", "Downloading torrents")
                    torrents: statusFilterStats.downloadingTorrents

                    onClicked: {
                        statusSwitch.statusFilter = TorrentsProxyModel.Downloading
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.Downloading
                    }
                }

                FilterTorrentsListItem {
                    current: torrentsProxyModel.statusFilter === TorrentsProxyModel.Seeding
                    text: qsTranslate("tremotesf", "Seeding", "Seeding torrents")
                    torrents: statusFilterStats.seedingTorrents

                    onClicked: {
                        statusSwitch.statusFilter = TorrentsProxyModel.Seeding
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.Seeding
                    }
                }

                FilterTorrentsListItem {
                    current: torrentsProxyModel.statusFilter === TorrentsProxyModel.Paused
                    text: qsTranslate("tremotesf", "Paused", "Paused torrents")
                    torrents: statusFilterStats.pausedTorrents

                    onClicked: {
                        statusSwitch.statusFilter = TorrentsProxyModel.Paused
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.Paused
                    }
                }

                FilterTorrentsListItem {
                    current: torrentsProxyModel.statusFilter === TorrentsProxyModel.Checking
                    text: qsTranslate("tremotesf", "Checking", "Checking torrents")
                    torrents: statusFilterStats.checkingTorrents

                    onClicked: {
                        statusSwitch.statusFilter = TorrentsProxyModel.Checking
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.Checking
                    }
                }

                FilterTorrentsListItem {
                    current: torrentsProxyModel.statusFilter === TorrentsProxyModel.Errored
                    text: qsTranslate("tremotesf", "Errored")
                    torrents: statusFilterStats.erroredTorrents

                    onClicked: {
                        statusSwitch.statusFilter = TorrentsProxyModel.Errored
                        torrentsProxyModel.statusFilter = TorrentsProxyModel.Errored
                    }
                }
            }

            TextSwitch {
                id: directoriesSwitch

                property string directory

                checked: true
                text: qsTranslate("tremotesf", "Directories")

                onCheckedChanged: {
                    if (checked) {
                        torrentsProxyModel.downloadDirectory = directory
                    } else {
                        torrentsProxyModel.downloadDirectory = String()
                    }
                }
            }

            Column {
                width: parent.width
                visible: directoriesSwitch.checked

                Repeater {
                    model: DownloadDirectoriesModel {
                        rpc: rootWindow.rpc
                        torrentsProxyModel: torrentsView.model
                    }
                    delegate: FilterTorrentsListItem {
                        property var modelData: model

                        current: modelData.directory === torrentsProxyModel.downloadDirectory
                        text: (modelData.index > 0) ? modelData.directory : qsTranslate("tremotesf", "All")
                        torrents: modelData.torrents

                        onClicked: {
                            directoriesSwitch.directory = modelData.directory
                            torrentsProxyModel.downloadDirectory = modelData.directory
                        }
                    }
                }
            }

            TextSwitch {
                id: trackersSwitch

                property string tracker

                checked: true
                text: qsTranslate("tremotesf", "Trackers")

                onCheckedChanged: {
                    if (checked) {
                        torrentsProxyModel.tracker = tracker
                    } else {
                        torrentsProxyModel.tracker = String()
                    }
                }
            }

            Column {
                width: parent.width
                visible: trackersSwitch.checked

                Repeater {
                    model: AllTrackersModel {
                        rpc: rootWindow.rpc
                        torrentsProxyModel: torrentsView.model
                    }
                    delegate: FilterTorrentsListItem {
                        property var modelData: model

                        current: modelData.tracker === torrentsProxyModel.tracker
                        text: (modelData.index > 0) ? modelData.tracker : qsTranslate("tremotesf", "All")
                        torrents: modelData.torrents

                        onClicked: {
                            trackersSwitch.tracker = modelData.tracker
                            torrentsProxyModel.tracker = modelData.tracker
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
