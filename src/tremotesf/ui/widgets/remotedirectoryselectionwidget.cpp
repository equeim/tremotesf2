// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "remotedirectoryselectionwidget.h"

#include "libtremotesf/serversettings.h"
#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/rpc/servers.h"

#include <QCollator>
#include <QComboBox>
#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

namespace tremotesf {
    namespace {
        inline QString chopTrailingSeparator(QString directory) {
            if (directory.endsWith('/')) {
                directory.chop(1);
            }
            return directory;
        }
    }

    RemoteDirectorySelectionWidget::RemoteDirectorySelectionWidget(
        const QString& directory, const Rpc* rpc, bool comboBox, QWidget* parent
    )
        : FileSelectionWidget(true, QString(), rpc->isLocal(), comboBox, parent), mRpc(rpc) {
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
                    QMessageBox::warning(
                        this,
                        qApp->translate("tremotesf", "Error"),
                        qApp->translate("tremotesf", "Selected directory should be inside mounted directory")
                    );
                } else {
                    setText(directory);
                    setFileDialogDirectory(filePath);
                }
            });
        }
    }

    void RemoteDirectorySelectionWidget::updateComboBox(const QString& setAsCurrent) {
        if (!textComboBox()) {
            return;
        }

        QStringList currentServerAddTorrentDialogDirectories;
        {
            const auto saved = Servers::instance()->currentServerAddTorrentDialogDirectories();
            currentServerAddTorrentDialogDirectories.reserve(
                saved.size() + static_cast<QStringList::size_type>(mRpc->torrents().size()) + 1
            );
            for (const auto& directory : saved) {
                currentServerAddTorrentDialogDirectories.push_back(chopTrailingSeparator(directory));
            }
        }
        const bool wasEmpty = currentServerAddTorrentDialogDirectories.empty();

        for (const auto& torrent : mRpc->torrents()) {
            currentServerAddTorrentDialogDirectories.push_back(chopTrailingSeparator(torrent->downloadDirectory()));
        }
        currentServerAddTorrentDialogDirectories.push_back(
            chopTrailingSeparator(mRpc->serverSettings()->downloadDirectory())
        );

        currentServerAddTorrentDialogDirectories.removeDuplicates();

        QCollator collator;
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        collator.setNumericMode(true);
        std::sort(
            currentServerAddTorrentDialogDirectories.begin(),
            currentServerAddTorrentDialogDirectories.end(),
            [&collator](const auto& first, const auto& second) { return collator.compare(first, second) < 0; }
        );

        if (wasEmpty && !currentServerAddTorrentDialogDirectories.empty()) {
            Servers::instance()->setCurrentServerAddTorrentDialogDirectories(currentServerAddTorrentDialogDirectories);
        }

        textComboBox()->clear();
        textComboBox()->addItems(currentServerAddTorrentDialogDirectories);
        textComboBox()->setCurrentIndex(currentServerAddTorrentDialogDirectories.indexOf(setAsCurrent));
    }
}
