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

import Sailfish.Silica 1.0

BackgroundItem {
    property bool current
    property alias text: label.text
    property alias torrents: torrentsLabel.text

    Label {
        id: label

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin * 2
            right: torrentsLabel.left
            verticalCenter: parent.verticalCenter
        }
        truncationMode: TruncationMode.Fade
        color: (highlighted || current) ? Theme.highlightColor : Theme.primaryColor
    }

    Label {
        id: torrentsLabel

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        color: Theme.highlightColor
    }
}
