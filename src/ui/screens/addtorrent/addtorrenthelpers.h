// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ADDTORRENTHELPERS_H
#define TREMOTESF_ADDTORRENTHELPERS_H

#include <QString>

#include "bencodeparser.h"
#include "rpc/torrent.h"

class QDialog;
class QWidget;

namespace tremotesf {
    class Rpc;

    struct AddTorrentParameters {
        QString downloadDirectory;
        TorrentData::Priority priority;
        bool startAfterAdding;
        bool deleteTorrentFile;
        bool moveTorrentFileToTrash;
    };

    AddTorrentParameters getAddTorrentParameters(Rpc* rpc);
    AddTorrentParameters getInitialAddTorrentParameters(Rpc* rpc);

    QDialog* askForMergingTrackers(Torrent* torrent, std::vector<std::set<QString>> trackers, QWidget* parent);

    QString bencodeErrorString(bencode::Error::Type type);
}

#endif // TREMOTESF_ADDTORRENTHELPERS_H
