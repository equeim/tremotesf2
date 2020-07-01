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
import Nemo.Notifications 1.0

import harbour.tremotesf 1.0

Page {
    id: mainPage

    allowedOrientations: defaultAllowedOrientations

    SearchPanel {
        id: searchPanel
    }

    SilicaListView {
        id: torrentsView

        property alias selectionModel: selectionModel

        anchors {
            fill: parent
            topMargin: searchPanel.visibleSize
            bottomMargin: Math.max(bottomPanel.visibleSize, selectionPanel.visibleSize)
        }
        clip: true

        header: PageHeader {
            title: "Tremotesf"
            description: {
                if (Servers.hasServers) {
                    //: %s is server's name, %2 is server's address
                    return qsTranslate("tremotesf", "%1 (%2)").arg(Servers.currentServerName).arg(Servers.currentServerAddress)
                }
                return String()
            }
        }

        model: TorrentsProxyModel {
            id: torrentsProxyModel

            sourceModel: TorrentsModel {
                id: torrentsModel
                rpc: rootWindow.rpc
            }

            Component.onCompleted: {
                sortRole = Settings.torrentsSortRole
                sort(0, Settings.torrentsSortOrder)
            }

            Component.onDestruction: {
                Settings.torrentsSortOrder = sortOrder
                Settings.torrentsSortRole = sortRole
            }
        }
        delegate: TorrentDelegate { }

        SelectionModel {
            id: selectionModel
            model: torrentsProxyModel
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                text: qsTranslate("tremotesf", "Tools")
                onClicked: pageStack.push("ToolsPage.qml")
            }

            MenuItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Add Torrent Link...")
                onClicked: pageStack.push("AddTorrentLinkDialog.qml")
            }

            MenuItem {
                enabled: rpc.connected
                text: qsTranslate("tremotesf", "Add Torrent File...")
                onClicked: {
                    var dialog = pageStack.push("FileSelectionDialog.qml", {"acceptDestination": Qt.createComponent("AddTorrentFileDialog.qml"),
                                                                            "acceptDestinationAction": PageStackAction.Replace,
                                                                            "nameFilters": ["*.torrent"],
                                                                            "automaticAccept": false})
                    dialog.accepted.connect(function() {
                        dialog.acceptDestinationInstance.filePath = dialog.filePath
                    })
                }
            }

            MenuItem {
                enabled: Servers.hasServers
                text: rpc.status !== Rpc.Disconnected ? qsTranslate("tremotesf", "Disconnect")
                                                      : qsTranslate("tremotesf", "Connect")
                onClicked: {
                    if (rpc.status !== Rpc.Disconnected) {
                        var disconnect = function() {
                            rpc.disconnect()
                            pullDownMenu.activeChanged.disconnect(disconnect)
                        }
                        pullDownMenu.activeChanged.connect(disconnect)
                    } else {
                        rpc.connect()
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: !rpc.connected
            text: {
                if (Servers.hasServers) {
                    return rpc.statusString
                }
                return qsTranslate("tremotesf", "No servers")
            }
        }

        ViewPlaceholder {
            enabled: rpc.connected && !torrentsView.count
            text: qsTranslate("tremotesf", "No torrents")
        }

        VerticalScrollDecorator { }
    }

    BottomPanel {
        id: bottomPanel
    }

    SelectionPanel {
        id: selectionPanel
        selectionModel: torrentsView.selectionModel
        text: qsTranslate("tremotesf", "%Ln torrents selected", String(), selectionModel.selectedIndexesCount)

        PushUpMenu {
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Start")
                onClicked: rpc.startTorrents(torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes)))
            }
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Start Now")
                onClicked: rpc.startTorrentsNow(torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes)))
            }
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Pause")
                onClicked: rpc.pauseTorrents(torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes)))
            }
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Remove")
                onClicked: pageStack.push("RemoveTorrentsDialog.qml",
                                          {"ids": torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes))})
            }
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Set Location")
                onClicked: pageStack.push("SetLocationDialog.qml",
                                          {"directory": torrentsModel.torrentAtIndex(torrentsProxyModel.sourceIndex(selectionModel.selectedIndexes[0])).downloadDirectory,
                                           "ids": torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes))})
            }
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Check Local Data")
                onClicked: rpc.checkTorrents(torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes)))
            }
            MenuItem {
                enabled: selectionModel.hasSelection
                text: qsTranslate("tremotesf", "Reannounce")
                onClicked: rpc.reannounceTorrents(torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes)))
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

    Connections {
        target: rpc

        onConnectedChanged: {
            if (!rpc.connected && (rpc.error !== Rpc.NoError) && Settings.notificationOnDisconnecting) {
                notification.publishGeneric(qsTranslate("tremotesf", "Disconnected"), rpc.statusString)
            }
        }

        onAddedNotificationRequested: {
            var count = hashStrings.length
            if (count === 1) {
                notification.addedTorrent(names[0], hashStrings[0], true)
            } else {
                for (var i = 0; i < count; ++i) {
                    notification.addedTorrent(names[i], hashStrings[i], false)
                }
                notification.publishGeneric(qsTranslate("tremotesf", "%Ln torrents added", String(), count))
            }
        }

        onFinishedNotificationRequested: {
            var count = hashStrings.length
            if (count === 1) {
                notification.finishedTorrent(names[0], hashStrings[0], true)
            } else {
                for (var i = 0; i < count; ++i) {
                    notification.finishedTorrent(names[i], hashStrings[i], false)
                }
                notification.publishGeneric(qsTranslate("tremotesf", "%Ln torrents finished", String(), count))
            }
        }

        onTorrentAddDuplicate: notification.publishGeneric(qsTranslate("tremotesf", "Error adding torrent"), qsTranslate("tremotesf", "This torrent is already added"))
        onTorrentAddError: notification.publishGeneric(qsTranslate("tremotesf", "Error adding torrent"), qsTranslate("tremotesf", "Error adding torrent"))
    }

    Notification {
        id: notification

        function publishGeneric(summary, body) {
            notification.summary = summary
            notification.previewSummary = summary
            if (body === undefined) {
                notification.body = String()
                notification.previewBody = String()
            } else {
                notification.body = body
                notification.previewBody = body
            }
            notification.remoteActions = [{
                "name": "default",
                "service": ipcServer.serviceName,
                "path": ipcServer.objectPath,
                "iface": ipcServer.interfaceName,
                "method": "ActivateWindow",
            }]
            notification.replacesId = 0
            notification.publish()
        }

        function addedTorrent(name, hashString, preview) {
            publishTorrent(qsTranslate("tremotesf", "Torrent added"), name, hashString, preview)
        }

        function finishedTorrent(name, hashString, preview) {
            publishTorrent(qsTranslate("tremotesf", "Torrent finished"), name, hashString, preview)
        }

        function publishTorrent(summary, body, hashString, preview) {
            notification.summary = summary
            notification.body = body
            if (preview) {
                notification.previewSummary = summary
                notification.previewBody = body
            } else {
                notification.previewSummary = String()
                notification.previewBody = String()
            }
            notification.remoteActions = [{
                "name": "default",
                "service": ipcServer.serviceName,
                "path": ipcServer.objectPath,
                "iface": ipcServer.interfaceName,
                "method": "OpenTorrentPropertiesPage",
                "arguments": [hashString]
            }]
            notification.replacesId = 0
            notification.publish()
        }
    }
}
