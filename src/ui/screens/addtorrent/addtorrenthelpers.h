// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ADDTORRENTHELPERS_H
#define TREMOTESF_ADDTORRENTHELPERS_H

#include <QString>

#include "rpc/torrent.h"

class QDialog;

namespace tremotesf {
    class Rpc;
    class Torrent;

    struct AddTorrentParameters {
        QString downloadDirectory;
        TorrentData::Priority priority;
        bool startAfterAdding;
        bool deleteTorrentFile;
        bool moveTorrentFileToTrash;
    };

    AddTorrentParameters getAddTorrentParameters(Rpc* rpc);
    AddTorrentParameters getInitialAddTorrentParameters(Rpc* rpc);

    void deleteTorrentFile(const QString& filePath, bool moveToTrash);

    QDialog* askForMergingTrackers(Torrent* torrent, std::vector<std::set<QString>> trackers, QWidget* parent);
}

#endif // TREMOTESF_ADDTORRENTHELPERS_H
