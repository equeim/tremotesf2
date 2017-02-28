/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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
                title: qsTranslate("tremotesf", "About")
            }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: {
                    var iconSize = Theme.iconSizeExtraLarge
                    if (iconSize < 108) {
                        iconSize = 86
                    } else if (iconSize < 128) {
                        iconSize = 108
                    } else if (iconSize < 256) {
                        iconSize = 128
                    } else {
                        iconSize = 256
                    }
                    return "/usr/share/icons/hicolor/%1x%2/apps/harbour-tremotesf.png".arg(iconSize).arg(iconSize)
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingMedium
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: Theme.fontSizeLarge
                text: "Tremotesf %1".arg(Qt.application.version)
            }

            Label {
                horizontalAlignment: implicitWidth > width ? Text.AlignLeft : Text.AlignHCenter
                width: parent.width
                font.pixelSize: Theme.fontSizeExtraSmall
                text: "<style type=\"text/css\">A { color: %1; }</style>".arg(Theme.highlightColor) +
                      "\u00a9 2015-2017 Alexey Rochev &lt;<a href=\"mailto:equeim@gmail.com\">equeim@gmail.com</a>&gt;"
                textFormat: Text.RichText
                truncationMode: TruncationMode.Fade

                onLinkActivated: Qt.openUrlExternally(link)
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            Column {
                width: parent.width
                spacing: Theme.paddingMedium

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Theme.buttonWidthLarge
                    text: qsTranslate("tremotesf", "Source Code")
                    onClicked: Qt.openUrlExternally("https://github.com/equeim/tremotesf2")
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Theme.buttonWidthLarge
                    text: qsTranslate("tremotesf", "Translations")
                    onClicked: Qt.openUrlExternally("https://www.transifex.com/equeim/tremotesf")
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Theme.buttonWidthLarge
                    text: qsTranslate("tremotesf", "Authors")

                    onClicked: pageStack.push(authorsPageComponent)

                    Component {
                        id: authorsPageComponent

                        Page {
                            allowedOrientations: defaultAllowedOrientations

                            Column {
                                id: column
                                width: parent.width

                                PageHeader {
                                    title: qsTranslate("tremotesf", "Authors")
                                }

                                Label {
                                    anchors {
                                        left: parent.left
                                        leftMargin: Theme.horizontalPageMargin
                                        right: parent.right
                                        rightMargin: Theme.horizontalPageMargin
                                    }
                                    text: "<style type=\"text/css\">A { color: %1; }</style>".arg(Theme.highlightColor) +
                                          "Alexey Rochev &lt;<a href=\"mailto:equeim@gmail.com\">equeim@gmail.com</a>&gt;"
                                    textFormat: Text.RichText
                                    onLinkActivated: Qt.openUrlExternally(link)
                                }

                                Label {
                                    x: Theme.horizontalPageMargin
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.secondaryColor
                                    text: qsTranslate("tremotesf", "Maintainer")
                                }
                            }
                        }
                    }
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Theme.buttonWidthLarge
                    text: qsTranslate("tremotesf", "Translators")

                    onClicked: pageStack.push(translatorsPageComponent)

                    Component {
                        id: translatorsPageComponent

                        Page {
                            allowedOrientations: defaultAllowedOrientations

                            SilicaFlickable {
                                anchors.fill: parent
                                contentHeight: column.height

                                Column {
                                    id: column
                                    width: parent.width

                                    PageHeader {
                                        title: qsTranslate("tremotesf", "Translators")
                                    }

                                    Label {
                                        anchors {
                                            left: parent.left
                                            leftMargin: Theme.horizontalPageMargin
                                            right: parent.right
                                            rightMargin: Theme.horizontalPageMargin
                                        }
                                        font.pixelSize: Theme.fontSizeSmall
                                        text: {
                                            var translators = "<style type=\"text/css\">A { color: %1; }</style>".arg(Theme.highlightColor)
                                            translators += Utils.translators
                                            return translators
                                        }
                                        wrapMode: Text.WordWrap
                                        textFormat: Text.RichText

                                        onLinkActivated: Qt.openUrlExternally(link)
                                    }
                                }

                                VerticalScrollDecorator { }
                            }
                        }
                    }
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Theme.buttonWidthLarge
                    text: qsTranslate("tremotesf", "License")

                    onClicked: pageStack.push(licensePageComponent)

                    Component {
                        id: licensePageComponent

                        Page {
                            allowedOrientations: defaultAllowedOrientations

                            SilicaFlickable {
                                anchors.fill: parent
                                contentHeight: column.height + Theme.paddingLarge

                                Column {
                                    id: column
                                    width: parent.width

                                    PageHeader {
                                        title: qsTranslate("tremotesf", "License")
                                    }

                                    Label {
                                        anchors {
                                            left: parent.left
                                            leftMargin: Theme.horizontalPageMargin
                                            right: parent.right
                                            rightMargin: Theme.horizontalPageMargin
                                        }
                                        font.pixelSize: Theme.fontSizeSmall
                                        text: {
                                            var license = "<style type=\"text/css\">A { color: %1; }</style>".arg(Theme.highlightColor)
                                            license += Utils.license
                                            return license
                                        }
                                        textFormat: Text.RichText
                                        wrapMode: Text.WordWrap

                                        onLinkActivated: Qt.openUrlExternally(link)
                                    }
                                }

                                VerticalScrollDecorator { }
                            }
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }

        VerticalScrollDecorator { }
    }
}
