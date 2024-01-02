// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_ADDTORRENTHELPERS_H
#define TREMOTESF_ADDTORRENTHELPERS_H

#include <QString>

#include "rpc/torrent.h"

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

    void deleteTorrentFile(const QString& filePath, bool moveToTrash);
}

#endif // TREMOTESF_ADDTORRENTHELPERS_H
