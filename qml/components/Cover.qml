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

CoverBackground {
    Image {
        anchors.fill: parent
        opacity: 0.15
        source: "../../cover.svg"
        fillMode: Image.PreserveAspectCrop
    }

    Item {
        anchors {
            left: parent.left
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.paddingLarge
            top: parent.top
            topMargin: Theme.paddingLarge
            bottom: parent.bottom
        }

        Column {
            width: parent.width
            spacing: Theme.paddingLarge

            Label {
                id: statusLabel
                visible: !rpc.connected
                width: parent.width
                wrapMode: Text.Wrap
                maximumLineCount: 2
                truncationMode: TruncationMode.Elide
                text: {
                    if (Accounts.hasAccounts) {
                        return rpc.statusString
                    }
                    return qsTranslate("tremotesf", "No accounts")
                }
            }

            Column {
                visible: !statusLabel.visible
                width: parent.width
                spacing: Theme.paddingMedium

                Item {
                    width: parent.width
                    height: Math.max(downArrow.height, downloadSpeedLabel.height)

                    Label {
                        id: downArrow
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: Theme.fontSizeLarge
                        text: "\u2193"
                    }

                    Label {
                        id: downloadSpeedLabel
                        anchors {
                            left: downArrow.right
                            leftMargin: Theme.paddingSmall
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        text: Utils.formatByteSpeed(rpc.serverStats.downloadSpeed)
                        truncationMode: TruncationMode.Fade
                    }
                }

                Item {
                    width: parent.width
                    height: Math.max(upArrow.height, uploadSpeedLabel.height)

                    Label {
                        id: upArrow
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: Theme.fontSizeLarge
                        text: "\u2191"
                    }

                    Label {
                        id: uploadSpeedLabel
                        anchors {
                            left: upArrow.right
                            leftMargin: Theme.paddingSmall
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                        }
                        text: Utils.formatByteSpeed(rpc.serverStats.uploadSpeed)
                        truncationMode: TruncationMode.Fade
                    }
                }
            }

            Column {
                visible: Accounts.hasAccounts
                width: parent.width

                Label {
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: Accounts.currentAccountName
                }

                Label {
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    text: Accounts.currentAccountAddress
                }
            }
        }
    }

    CoverActionList {
        enabled: Accounts.hasAccounts

        CoverAction {
            iconSource: rpc.connected ? "image://theme/icon-cover-cancel" : "image://theme/icon-cover-sync"
            onTriggered: {
                if (rpc.connected) {
                    rpc.disconnect()
                } else {
                    rpc.connect()
                }
            }
        }
    }
}
