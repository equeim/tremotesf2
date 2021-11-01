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
    id: filterTorrentsPage

    allowedOrientations: defaultAllowedOrientations

    property TorrentsProxyModel torrentsProxyModel

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
                text: qsTranslate("tremotesf", "Status")
                checked: torrentsProxyModel.statusFilterEnabled
                onClicked: torrentsProxyModel.statusFilterEnabled = !torrentsProxyModel.statusFilterEnabled
            }

            Column {
                width: parent.width
                visible: statusSwitch.checked

                Repeater {
                    model: StatusFiltersModel {
                        rpc: rootWindow.rpc
                        torrentsProxyModel: filterTorrentsPage.torrentsProxyModel
                    }
                    delegate: FilterTorrentsListItem {
                        property var modelData: model

                        current: modelData.filter === torrentsProxyModel.statusFilter
                        text: modelData.filterName
                        elide: Text.ElideMiddle
                        torrents: modelData.torrents

                        onClicked: torrentsProxyModel.statusFilter = modelData.filter
                    }
                }
            }

            TextSwitch {
                id: directoriesSwitch
                text: qsTranslate("tremotesf", "Directories")
                checked: torrentsProxyModel.downloadDirectoryFilterEnabled
                onClicked: torrentsProxyModel.downloadDirectoryFilterEnabled = !torrentsProxyModel.downloadDirectoryFilterEnabled
            }

            Column {
                width: parent.width
                visible: directoriesSwitch.checked

                Repeater {
                    model: DownloadDirectoriesModel {
                        rpc: rootWindow.rpc
                        torrentsProxyModel: filterTorrentsPage.torrentsProxyModel
                    }
                    delegate: FilterTorrentsListItem {
                        property var modelData: model

                        current: modelData.directory === torrentsProxyModel.downloadDirectoryFilter
                        text: modelData.directory ? modelData.directory : qsTranslate("tremotesf", "All")
                        elide: Text.ElideMiddle
                        torrents: modelData.torrents

                        onClicked: torrentsProxyModel.downloadDirectoryFilter = modelData.directory
                    }
                }
            }

            TextSwitch {
                id: trackersSwitch
                text: qsTranslate("tremotesf", "Trackers")
                checked: torrentsProxyModel.trackerFilterEnabled
                onClicked: torrentsProxyModel.trackerFilterEnabled = !torrentsProxyModel.trackerFilterEnabled
            }

            Column {
                width: parent.width
                visible: trackersSwitch.checked

                Repeater {
                    model: AllTrackersModel {
                        rpc: rootWindow.rpc
                        torrentsProxyModel: filterTorrentsPage.torrentsProxyModel
                    }
                    delegate: FilterTorrentsListItem {
                        property var modelData: model

                        current: modelData.tracker === torrentsProxyModel.trackerFilter
                        text: modelData.tracker ? modelData.tracker : qsTranslate("tremotesf", "All")
                        torrents: modelData.torrents

                        onClicked: torrentsProxyModel.trackerFilter = modelData.tracker
                    }
                }
            }
        }

        VerticalScrollDecorator { }
    }
}
