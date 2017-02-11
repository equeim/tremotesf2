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

ListItem {
    property alias label: detailItem.label
    property alias value: detailItem.value

    contentHeight: detailItem.height
    menu: Component {
        ContextMenu {
            MenuItem {
                text: qsTranslate("tremotesf", "Copy")
                onClicked: Clipboard.text = value
            }
        }
    }

    DetailItem {
        id: detailItem
    }
}
