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

FileSelectionItem {
    property bool mounted: !rpc.local && Servers.currentServerHasMountedDirectories
    property bool settingTextFromDialog: false

    selectionButtonEnabled: rpc.local || Servers.currentServerHasMountedDirectories
    showFiles: false

    connectTextFieldWithDialog: rpc.local
    fileDialogCanAccept: mounted ? Servers.isUnderCurrentServerMountedDirectory(fileDialogDirectory) : true
    fileDialogErrorString: qsTranslate("tremotesf", "Selected directory is not inside mounted directory")

    Component.onCompleted: {
        if (mounted && !fileDialogDirectory) {
            fileDialogDirectory = Servers.firstLocalDirectory
        }
    }

    onTextChanged: {
        var path = text.trim()
        if (mounted && !settingTextFromDialog) {
            var directory = Servers.fromRemoteToLocalDirectory(path)
            if (directory) {
                fileDialogDirectory = directory
            }
        }
    }

    onDialogAccepted: {
        if (mounted) {
            settingTextFromDialog = true
            text = Servers.fromLocalToRemoteDirectory(filePath)
            settingTextFromDialog = false
        }
    }
}
