// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QCheckBox>
#include <QMessageBox>
#include <QPointer>

#include "addtorrenthelpers.h"

#include "settings.h"
#include "rpc/rpc.h"
#include "rpc/servers.h"
#include "rpc/serversettings.h"

namespace tremotesf {
    AddTorrentParameters getAddTorrentParameters(Rpc* rpc) {
        auto* const settings = Settings::instance();
        auto* const serverSettings = rpc->serverSettings();
        return {
            .downloadDirectory =
                [&] {
                    const auto rememberAddTorrentParameters = settings->get_rememberAddTorrentParameters();
                    if (rememberAddTorrentParameters) {
                        const auto lastDir = Servers::instance()->currentServerLastDownloadDirectory(serverSettings);
                        if (!lastDir.isEmpty()) {
                            return lastDir;
                        }
                    }
                    return serverSettings->data().downloadDirectory;
                }(),
            .priority = settings->get_lastAddTorrentPriority(),
            .startAfterAdding = settings->get_lastAddTorrentStartAfterAdding(),
            .deleteTorrentFile = settings->get_lastAddTorrentDeleteTorrentFile(),
            .moveTorrentFileToTrash = settings->get_lastAddTorrentMoveTorrentFileToTrash()
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

    QDialog* askForMergingTrackers(Torrent* torrent, std::vector<std::set<QString>> trackers, QWidget* parent) {
        auto* const settings = Settings::instance();
        QMessageBox* messageBox{};
        if (settings->get_askForMergingTrackersWhenAddingExistingTorrent()) {
            messageBox = new QMessageBox(
                QMessageBox::Question,
                //: Dialog title
                qApp->translate("tremotesf", "Merge trackers?"),
                qApp->translate("tremotesf", "Torrent «%1» is already added, merge trackers?")
                    .arg(torrent->data().name),
                QMessageBox::Ok | QMessageBox::Cancel,
                parent
            );
            messageBox->button(QMessageBox::Ok)->setText(qApp->translate("tremotesf", "Merge"));
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
                        settings->set_mergeTrackersWhenAddingExistingTorrent(result == QMessageBox::Ok);
                        settings->set_askForMergingTrackersWhenAddingExistingTorrent(false);
                    }
                }
            );

        } else if (settings->get_mergeTrackersWhenAddingExistingTorrent()) {
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
