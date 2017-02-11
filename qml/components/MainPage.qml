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
import org.nemomobile.dbus 2.0
import org.nemomobile.notifications 1.0

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
                if (Accounts.hasAccounts) {
                    return qsTranslate("tremotesf", "%1 (%2)").arg(Accounts.currentAccountName).arg(Accounts.currentAccountAddress)
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
                enabled: Accounts.hasAccounts
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
                if (Accounts.hasAccounts) {
                    return rpc.statusString
                }
                return qsTranslate("tremotesf", "No accounts")
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
        text: qsTranslate("tremotesf", "%n torrent(s) selected", String(), selectionModel.selectedIndexesCount)

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
                text: qsTranslate("tremotesf", "Check Local Data")
                onClicked: rpc.checkTorrents(torrentsModel.idsFromIndexes(torrentsProxyModel.sourceIndexes(selectionModel.selectedIndexes)))
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

    DBusAdaptor {
        id: dbusAdaptor

        function activateWindow() {
            activate()
        }

        function openTorrentPropertiesPage(torrent) {
            activate()
            pageStack.pop(mainPage, PageStackAction.Immediate)
            pageStack.push("TorrentPropertiesPage.qml", {"torrent": torrentsModel.torrentByName(torrent)}, PageStackAction.Immediate)
        }

        service: "org.tremotesf"
        path: "/"
        iface: "org.tremotesf"
    }

    Connections {
        target: rpc

        onConnectedChanged: {
            if (!rpc.connected && (rpc.error !== Rpc.NoError) && Settings.notificationOnDisconnecting) {
                notification.summary = qsTranslate("tremotesf", "Disconnected")
                notification.body = rpc.statusString
                notification.previewSummary = notification.summary
                notification.previewBody = notification.body
                notification.remoteActions = [{
                    "name": "default",
                    "service": dbusAdaptor.service,
                    "path": dbusAdaptor.path,
                    "iface": dbusAdaptor.iface,
                    "method": "activateWindow",
                }]
                notification.replacesId = 0
                notification.publish()
            }
        }

        onTorrentAdded: {
            if (!Settings.notificationOnAddingTorrent) {
                return
            }

            notification.summary = qsTranslate("tremotesf", "Torrent added")
            notification.body = torrent
            notification.previewSummary = notification.summary
            notification.previewBody = notification.body
            notification.remoteActions = [{
                "name": "default",
                "service": dbusAdaptor.service,
                "path": dbusAdaptor.path,
                "iface": dbusAdaptor.iface,
                "method": "openTorrentPropertiesPage",
                "arguments": [torrent]
            }]
            notification.replacesId = 0
            notification.publish()
        }

        onTorrentFinished: {
            if (!Settings.notificationOfFinishedTorrents) {
                return
            }

            notification.summary = qsTranslate("tremotesf", "Torrent finished")
            notification.body = torrent
            notification.previewSummary = notification.summary
            notification.previewBody = notification.body
            notification.remoteActions = [{
                "name": "default",
                "service": dbusAdaptor.service,
                "path": dbusAdaptor.path,
                "iface": dbusAdaptor.iface,
                "method": "openTorrentPropertiesPage",
                "arguments": [torrent]
            }]
            notification.replacesId = 0
            notification.publish()
        }
    }

    Notification {
        id: notification
    }
}
