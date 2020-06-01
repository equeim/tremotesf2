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

import Sailfish.Silica 1.0
import harbour.tremotesf 1.0

PasswordField {
    id: formPasswordField

    readonly property bool _pageIsDialog: (typeof pageStack.currentPage.canAccept) === "boolean"
    property var _nextTextInput

    activeFocusOnTab: true

    onActiveFocusChanged: {
        if (activeFocus) {
            _nextTextInput = Utils.nextItemInFocusChainNotLooping(pageStack.currentPage, formPasswordField)
        }
    }

    EnterKey.iconSource: _nextTextInput ? "image://theme/icon-m-enter-next" : (_pageIsDialog ? "image://theme/icon-m-enter-accept" : "image://theme/icon-m-enter-close")
    EnterKey.onClicked: {
        if (_nextTextInput) {
            _nextTextInput.forceActiveFocus()
        } else if (_pageIsDialog) {
            pageStack.currentPage.accept()
        } else {
            focus = false
        }
    }
}
