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

#include "remotedirectoryselectionwidget.h"

#include "servers.h"

#include <QCoreApplication>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

namespace tremotesf
{
    RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget(const QString& directory, bool serverIsLocal, QWidget* parent)
        : FileSelectionWidget(true, QString(), serverIsLocal, parent)
    {
        const bool mounted = Servers::instance()->currentServerHasMountedDirectories();
        selectionButton()->setEnabled(serverIsLocal || mounted);
        setLineEditText(directory);
        if (mounted && !serverIsLocal) {
            const auto onTextEdited = [=](const QString& text) {
                const QString directory(Servers::instance()->fromRemoteToLocalDirectory(text));
                if (!directory.isEmpty()) {
                    setFileDialogDirectory(directory);
                }
            };
            QObject::connect(lineEdit(), &QLineEdit::textEdited, this, onTextEdited);
            const QString localDownloadDirectory(Servers::instance()->fromRemoteToLocalDirectory(directory));
            if (localDownloadDirectory.isEmpty()) {
                setFileDialogDirectory(Servers::instance()->firstLocalDirectory());
            } else {
                setFileDialogDirectory(localDownloadDirectory);
            }

            QObject::connect(this, &FileSelectionWidget::fileDialogAccepted, this, [=](const QString& filePath) {
                const QString directory(Servers::instance()->fromLocalToRemoteDirectory(filePath));
                if (directory.isEmpty()) {
                    QMessageBox::warning(this, qApp->translate("tremotesf", "Error"), qApp->translate("tremotesf", "Selected directory should be inside mounted directory"));
                } else {
                    setLineEditText(directory);
                    setFileDialogDirectory(filePath);
                }
            });
        }
    }
}
