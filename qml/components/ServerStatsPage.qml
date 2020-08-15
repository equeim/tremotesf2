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

import Sailfish.Silica 1.0
import QtQuick 2.2

import harbour.tremotesf 1.0

Page {
    property var currentSession: rpc.serverStats.currentSession
    property var total: rpc.serverStats.total

    allowedOrientations: defaultAllowedOrientations

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            DisconnectedHeader { }

            PageHeader {
                title: qsTranslate("tremotesf", "Server Stats")
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Current session")
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Downloaded")
                value: Utils.formatByteSize(currentSession.downloaded)
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Uploaded")
                value: Utils.formatByteSize(currentSession.uploaded)
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Ratio")
                value: Utils.formatRatio(currentSession.downloaded, currentSession.uploaded)
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Duration")
                value: Utils.formatEta(currentSession.duration)
            }

            SectionHeader {
                text: qsTranslate("tremotesf", "Total")
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Downloaded")
                value: Utils.formatByteSize(total.downloaded)
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Uploaded")
                value: Utils.formatByteSize(total.uploaded)
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Ratio")
                value: Utils.formatRatio(total.downloaded, total.uploaded)
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Duration")
                value: Utils.formatEta(total.duration)
            }

            DetailItem {
                label: qsTranslate("tremotesf", "Started")
                value: qsTranslate("tremotesf", "%Ln times", String(), total.sessionCount)
            }
        }

        VerticalScrollDecorator { }
    }
}

