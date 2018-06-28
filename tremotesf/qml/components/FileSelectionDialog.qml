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

Dialog {
    id: fileSelectionDialog

    property string filePath

    property alias directory: directoryContentModel.directory
    property alias showFiles: directoryContentModel.showFiles
    property alias nameFilters: directoryContentModel.nameFilters

    property string errorString

    allowedOrientations: defaultAllowedOrientations
    canAccept: showFiles ? filePath : true

    Component.onCompleted: {
        if (!showFiles) {
            filePath = directory
        }
    }

    SilicaListView {
        id: listView

        anchors.fill: parent

        header: Column {
            width: listView.width

            onHeightChanged: {
                if (listView.contentY < 0) {
                    listView.positionViewAtBeginning()
                }
            }

            DialogHeader {
                title: showFiles ? qsTranslate("tremotesf", "Select File")
                                 : qsTranslate("tremotesf", "Select Directory")
            }

            ErrorHeader {
                visible: !canAccept && errorString
                padding: Theme.paddingMedium
                fontSize: Theme.fontSizeSmall
                text: errorString
            }

            FileListItem {
                visible: directoryContentModel.directory !== directoryContentModel.parentDirectory
                directory: true
                text: ".."
                onClicked: directoryContentModel.directory = directoryContentModel.parentDirectory
            }
        }

        model: DirectoryContentModel {
            id: directoryContentModel

            onDirectoryChanged: {
                listView.positionViewAtBeginning()
                if (!showFiles) {
                    filePath = directory
                }
            }
        }

        delegate: FileListItem {
            directory: model.directory
            text: model.name
            textColor: highlighted ? Theme.highlightColor : Theme.primaryColor

            onClicked: {
                if (model.directory) {
                    directoryContentModel.directory = model.path
                } else {
                    fileSelectionDialog.filePath = model.path
                    accept()
                }
            }
        }

        PullDownMenu {
            id: pullDownMenu

            MenuItem {
                text: qsTranslate("tremotesf", "SD Card")
                onClicked: {
                    var cd = function() {
                        directoryContentModel.directory = Utils.sdcardPath
                        pullDownMenu.activeChanged.disconnect(cd)
                    }
                    pullDownMenu.activeChanged.connect(cd)
                }
            }

            MenuItem {
                text: qsTranslate("tremotesf", "Home Directory")
                onClicked: {
                    var cd = function() {
                        directoryContentModel.directory = StandardPaths.home
                        pullDownMenu.activeChanged.disconnect(cd)
                    }
                    pullDownMenu.activeChanged.connect(cd)
                }
            }
        }

        ViewPlaceholder {
            enabled: !listView.count
            text: qsTranslate("tremotesf", "No files")
        }

        VerticalScrollDecorator { }
    }
}
