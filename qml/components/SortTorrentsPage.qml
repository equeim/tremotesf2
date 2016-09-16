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
    allowedOrientations: defaultAllowedOrientations

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTranslate("tremotesf", "Sort")
            }

            ComboBox {
                label: qsTranslate("tremotesf", "Order")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTranslate("tremotesf", "Ascending")
                        onClicked: torrentsProxyModel.sort(0, Qt.AscendingOrder)
                    }

                    MenuItem {
                        text: qsTranslate("tremotesf", "Descending")
                        onClicked: torrentsProxyModel.sort(0, Qt.DescendingOrder)
                    }
                }

                Component.onCompleted: {
                    if (torrentsProxyModel.sortOrder === Qt.DescendingOrder) {
                        currentIndex = 1
                    }
                }
            }

            Separator {
                width: parent.width
                color: Theme.secondaryColor
            }

            SortTorrentsListItem {
                sortRole: TorrentsModel.NameRole
                text: qsTranslate("tremotesf", "Name")
            }

            SortTorrentsListItem {
                sortRole: TorrentsModel.StatusRole
                text: qsTranslate("tremotesf", "Status")
            }

            SortTorrentsListItem {
                sortRole: TorrentsModel.PercentDoneRole
                text: qsTranslate("tremotesf", "Progress")
            }

            SortTorrentsListItem {
                sortRole: TorrentsModel.EtaRole
                text: qsTranslate("tremotesf", "ETA")
            }

            SortTorrentsListItem {
                sortRole: TorrentsModel.RatioRole
                text: qsTranslate("tremotesf", "Ratio")
            }

            SortTorrentsListItem {
                sortRole: TorrentsModel.TotalSizeRole
                text: qsTranslate("tremotesf", "Size")
            }

            SortTorrentsListItem {
                sortRole: TorrentsModel.AddedDateRole
                text: qsTranslate("tremotesf", "Added date")
            }
        }

        VerticalScrollDecorator { }
    }
}
