/*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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


import QtQuick 2.6
import Sailfish.Silica 1.0

import harbour.tremotesf 1.0

SelectionPanel {
    id: selectionPanel
    selectionModel: listView.selectionModel
    parentIndex: delegateModel.rootIndex
    text: qsTranslate("tremotesf", "%Ln files selected", String(), selectionModel.selectedIndexesCount)

    PushUpMenu {
        MenuItem {
            enabled: selectionModel.hasSelection
            text: qsTranslate("tremotesf", "Download", "File menu item, verb")
            onClicked: {
                filesModel.setFilesWanted(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), true)
                selectionPanel.hide()
            }
        }

        MenuItem {
            enabled: selectionModel.hasSelection
            text: qsTranslate("tremotesf", "Not Download")
            onClicked: {
                filesModel.setFilesWanted(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), false)
                selectionPanel.hide()
            }
        }

        MenuLabel {
            text: qsTranslate("tremotesf", "Priority")
        }

        MenuItem {
            enabled: selectionModel.hasSelection
            //: Priority
            text: qsTranslate("tremotesf", "High")
            onClicked: {
                filesModel.setFilesPriority(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), TorrentFilesModelEntry.HighPriority)
                selectionPanel.hide()
            }
        }

        MenuItem {
            enabled: selectionModel.hasSelection
            //: Priority
            text: qsTranslate("tremotesf", "Normal")
            onClicked: {
                filesModel.setFilesPriority(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), TorrentFilesModelEntry.NormalPriority)
                selectionPanel.hide()
            }
        }

        MenuItem {
            enabled: selectionModel.hasSelection
            //: Priority
            text: qsTranslate("tremotesf", "Low")
            onClicked: {
                filesModel.setFilesPriority(filesProxyModel.sourceIndexes(selectionModel.selectedIndexes), TorrentFilesModelEntry.LowPriority)
                selectionPanel.hide()
            }
        }
    }
}
