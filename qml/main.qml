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

import "components"

ApplicationWindow {
    id: rootWindow

    property Rpc rpc: Rpc {
        Component.onCompleted: {
            if (Servers.hasServers) {
                setServer(Servers.currentServer)
            }
        }
    }

    Connections {
        target: Servers
        onCurrentServerChanged: {
            if (Servers.hasServers) {
                rpc.setServer(Servers.currentServer)
                rpc.connect()
            } else {
                rpc.resetServer()
            }
        }
    }

    Connections {
        target: Settings
        onAutoReconnectChanged: {
            rpc.setAutoReconnect(Settings.autoReconnect)
        }
    }

    cover: Qt.resolvedUrl("components/Cover.qml")
    initialPage: mainPage

    Component.onCompleted: {
        var addTorrent = function() {
            if (files.length) {
                pageStack.push("components/AddTorrentFileDialog.qml",
                               {filePath: files[0]})
            } else if (urls.length) {
                pageStack.push("components/AddTorrentLinkDialog.qml", {url: urls[0]})
            }
            rpc.connectedChanged.disconnect(addTorrent)
        }
        rpc.connectedChanged.connect(addTorrent)

        rpc.setAutoReconnect(Settings.autoReconnect)

        if (Servers.hasServers) {
            if (Settings.connectOnStartup) {
                rpc.connect()
            }
        } else {
            var dialog = pageStack.push("components/ServerEditDialog.qml", {}, PageStackAction.Immediate)
            dialog.accepted.connect(function() {
                if (Settings.connectOnStartup) {
                    rpc.connect()
                }
            })
        }
    }

    Component.onDestruction: rpc.disconnect()

    MainPage {
        id: mainPage
    }

    Connections {
        target: ipcServer

        onWindowActivationRequested: {
            activate()
            if (torrentHash.length > 0) {
                pageStack.pop(mainPage, PageStackAction.Immediate)
                pageStack.push("components/TorrentPropertiesPage.qml", {"torrentHash": torrentHash,
                                                                        "torrent": rpc.torrentByHash(torrentHash)}, PageStackAction.Immediate)
            }
        }

        onTorrentsAddingRequested: {
            activate()
            pageStack.pop(mainPage, PageStackAction.Immediate)
            if (files.length > 0) {
                pageStack.push("components/AddTorrentFileDialog.qml",
                               {filePath: files[0]},
                               PageStackAction.Immediate)
            } else if (urls.length > 0) {
                pageStack.push("components/AddTorrentLinkDialog.qml", {"url": urls[0]}, PageStackAction.Immediate)
            }
        }
    }
}
