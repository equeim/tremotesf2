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
    id: webSeedersPage

    property var torrent: torrentPropertiesPage.torrent

    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: listView

        anchors.fill: parent

        header: Column {
            width: listView.width

            TorrentRemovedHeader {
                torrent: webSeedersPage.torrent
            }

            PageHeader {
                title: qsTranslate("tremotesf", "Web Seeders")
            }
        }

        model: BaseProxyModel {
            sourceModel: StringListModel {
                property var stringList: webSeedersPage.torrent ? webSeedersPage.torrent.webSeeders : null
                onStringListChanged: setStringList(stringList)
            }
            Component.onCompleted: sort()
        }

        delegate: ListItem {
            id: listItem

            property var modelData: model

            menu: Component {
                ContextMenu {
                    MenuItem {
                        text: qsTranslate("tremotesf", "Copy")
                        onClicked: Clipboard.text = modelData.display
                    }
                }
            }

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                color: listItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                text: modelData.display
                truncationMode: TruncationMode.Fade
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTranslate("tremotesf", "No web seeders")
        }

        VerticalScrollDecorator { }
    }
}
