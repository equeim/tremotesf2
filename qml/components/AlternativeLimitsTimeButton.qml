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

ValueButton {
    property date time

    value: Qt.formatTime(time, "hh:mm")

    onClicked: {
        var dialog = pageStack.push("Sailfish.Silica.TimePickerDialog", {"hour": time.getHours(),
                                                                         "minute": time.getMinutes()})
        dialog.accepted.connect(function() {
            print(dialog.time)
            time = dialog.time
        })
    }
}
