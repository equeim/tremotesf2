// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QCheckBox>
#include <QMessageBox>
#include <QPointer>

#include "addtorrenthelpers.h"

#include "fileutils.h"
#include "settings.h"
#include "log/log.h"
#include "rpc/rpc.h"
#include "rpc/servers.h"

namespace tremotesf {
    AddTorrentParameters getAddTorrentParameters(Rpc* rpc) {
        auto* const settings = Settings::instance();
        auto* const serverSettings = rpc->serverSettings();
        return {
            .downloadDirectory =
                [&] {
                    const auto lastDir = Servers::instance()->currentServerLastDownloadDirectory(serverSettings);
                    return !lastDir.isEmpty() ? lastDir : serverSettings->data().downloadDirectory;
                }(),
            .priority = settings->lastAddTorrentPriority(),
            .startAfterAdding = settings->lastAddTorrentStartAfterAdding(),
            .deleteTorrentFile = settings->lastAddTorrentDeleteTorrentFile(),
            .moveTorrentFileToTrash = settings->lastAddTorrentMoveTorrentFileToTrash()
        };
    }

    AddTorrentParameters getInitialAddTorrentParameters(Rpc* rpc) {
        auto* const serverSettings = rpc->serverSettings();
        return {
            .downloadDirectory = serverSettings->data().downloadDirectory,
            .priority = TorrentData::Priority::Normal,
            .startAfterAdding = serverSettings->data().startAddedTorrents,
            .deleteTorrentFile = false,
            .moveTorrentFileToTrash = true
        };
    }

    void deleteTorrentFile(const QString& filePath, bool moveToTrash) {
        try {
            if (moveToTrash) {
                try {
                    moveFileToTrash(filePath);
                } catch (const QFileError& e) {
                    warning().logWithException(e, "Failed to move torrent file to trash");
                    deleteFile(filePath);
                }
            } else {
                deleteFile(filePath);
            }
        } catch (const QFileError& e) {
            warning().logWithException(e, "Failed to delete torrent file");
        }
    }

    QDialog* askForMergingTrackers(Torrent* torrent, std::vector<std::set<QString>> trackers, QWidget* parent) {
        auto* const settings = Settings::instance();
        QMessageBox* messageBox{};
        if (settings->askForMergingTrackersWhenAddingExistingTorrent()) {
            messageBox = new QMessageBox(
                QMessageBox::Question,
                //: Dialog title
                qApp->translate("tremotesf", "Merge trackers?"),
                qApp->translate("tremotesf", "Torrent «%1» is already added, merge trackers?")
                    .arg(torrent->data().name),
                QMessageBox::Ok | QMessageBox::Cancel,
                parent
            );
            messageBox->setButtonText(QMessageBox::Ok, qApp->translate("tremotesf", "Merge"));
            messageBox->setCheckBox(new QCheckBox(qApp->translate("tremotesf", "Do not ask again"), messageBox));
            QObject::connect(
                messageBox,
                &QDialog::finished,
                messageBox,
                [=, torrent = QPointer(torrent), trackers = std::move(trackers)](int result) {
                    if (result == QMessageBox::Ok && torrent) {
                        torrent->addTrackers(trackers);
                    }
                    if (messageBox->checkBox()->isChecked()) {
                        settings->setMergeTrackersWhenAddingExistingTorrent(result == QMessageBox::Ok);
                        settings->setAskForMergingTrackersWhenAddingExistingTorrent(false);
                    }
                }
            );

        } else if (settings->mergeTrackersWhenAddingExistingTorrent()) {
            torrent->addTrackers(trackers);

            messageBox = new QMessageBox(
                QMessageBox::Information,
                //: Dialog title
                qApp->translate("tremotesf", "Merged trackers"),
                qApp->translate("tremotesf", "Merged trackers for torrent «%1»").arg(torrent->data().name),
                QMessageBox::Close,
                parent
            );
        } else {
            messageBox = new QMessageBox(
                QMessageBox::Warning,
                //: Dialog title
                qApp->translate("tremotesf", "Torrent already exists"),
                qApp->translate("tremotesf", "Torrent «%1» already exists").arg(torrent->data().name),
                QMessageBox::Close,
                parent
            );
        }
        messageBox->setAttribute(Qt::WA_DeleteOnClose);
        messageBox->setModal(false);
        messageBox->show();
        return messageBox;
    }

    QString bencodeErrorString(bencode::Error::Type type) {
        switch (type) {
        case bencode::Error::Type::Reading:
            return qApp->translate("tremotesf", "Error reading torrent file");
        case bencode::Error::Type::Parsing:
            return qApp->translate("tremotesf", "Error parsing torrent file");
        default:
            return {};
        }
    }

}
