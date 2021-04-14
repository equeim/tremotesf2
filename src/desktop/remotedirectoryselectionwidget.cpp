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

#include "libtremotesf/serversettings.h"
#include "libtremotesf/torrent.h"
#include "../trpc.h"
#include "servers.h"

#include <QCollator>
#include <QComboBox>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

namespace tremotesf
{
    namespace
    {
        inline QString chopTrailingSeparator(QString directory) {
            if (directory.endsWith(QLatin1Char('/'))) {
                directory.chop(1);
            }
            return directory;
        }
    }

    RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget(const QString& directory, const Rpc* rpc, bool comboBox, QWidget* parent)
        : FileSelectionWidget(true, QString(), rpc->isLocal(), comboBox, parent),
          mRpc(rpc)
    {
        const bool mounted = Servers::instance()->currentServerHasMountedDirectories();
        selectionButton()->setEnabled(rpc->isLocal() || mounted);
        setText(directory);
        if (mounted && !rpc->isLocal()) {
            QObject::connect(this, &FileSelectionWidget::textChanged, this, [=](const auto& text) {
                const QString directory(Servers::instance()->fromRemoteToLocalDirectory(text));
                if (!directory.isEmpty()) {
                    setFileDialogDirectory(directory);
                }
            });
            const QString localDownloadDirectory(Servers::instance()->fromRemoteToLocalDirectory(directory));
            if (localDownloadDirectory.isEmpty()) {
                setFileDialogDirectory(Servers::instance()->firstLocalDirectory());
            } else {
                setFileDialogDirectory(localDownloadDirectory);
            }

            QObject::connect(this, &FileSelectionWidget::fileDialogAccepted, this, [=](const auto& filePath) {
                const QString directory(Servers::instance()->fromLocalToRemoteDirectory(filePath));
                if (directory.isEmpty()) {
                    QMessageBox::warning(this, qApp->translate("tremotesf", "Error"), qApp->translate("tremotesf", "Selected directory should be inside mounted directory"));
                } else {
                    setText(directory);
                    setFileDialogDirectory(filePath);
                }
            });
        }
    }

    void RemoteDirectorySelectionWidget::updateComboBox(const QString& setAsCurrent)
    {
        if (!textComboBox()) {
            return;
        }

        QStringList currentServerAddTorrentDialogDirectories(Servers::instance()->currentServerAddTorrentDialogDirectories());
        const bool wasEmpty = currentServerAddTorrentDialogDirectories.empty();

        currentServerAddTorrentDialogDirectories.reserve(static_cast<int>(mRpc->torrents().size() + 1));
        currentServerAddTorrentDialogDirectories.push_back(chopTrailingSeparator(mRpc->serverSettings()->downloadDirectory()));
        for (const auto& torrent : mRpc->torrents()) {
            currentServerAddTorrentDialogDirectories.push_back(chopTrailingSeparator(torrent->downloadDirectory()));
        }
        currentServerAddTorrentDialogDirectories.removeDuplicates();
        QCollator collator;
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        collator.setNumericMode(true);
        std::sort(currentServerAddTorrentDialogDirectories.begin(),
                  currentServerAddTorrentDialogDirectories.end(),
                  [&collator](const auto& first, const auto& second) { return collator.compare(first, second) < 0; });

        if (wasEmpty && !currentServerAddTorrentDialogDirectories.empty()) {
            Servers::instance()->setCurrentServerAddTorrentDialogDirectories(currentServerAddTorrentDialogDirectories);
        }

        textComboBox()->clear();
        textComboBox()->addItems(currentServerAddTorrentDialogDirectories);
        textComboBox()->setCurrentIndex(currentServerAddTorrentDialogDirectories.indexOf(setAsCurrent));
    }
}
