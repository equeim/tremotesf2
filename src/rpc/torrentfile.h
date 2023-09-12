// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_TORRENTFILE_H
#define TREMOTESF_RPC_TORRENTFILE_H

#include <vector>
#include <QObject>
#include <QString>

#include "log/formatters.h"

class QJsonObject;

namespace tremotesf {
    struct TorrentFile {
        Q_GADGET
    public:
        enum class Priority { Low, Normal, High };
        Q_ENUM(Priority)

        explicit TorrentFile(int id, const QJsonObject& fileMap, const QJsonObject& fileStatsMap);
        bool update(const QJsonObject& fileStatsMap);

        int id{};

        std::vector<QString> path{};
        qint64 size{};
        qint64 completedSize{};
        Priority priority{};
        bool wanted{};
    };
}

SPECIALIZE_FORMATTER_FOR_Q_ENUM(tremotesf::TorrentFile::Priority)

#endif // TREMOTESF_RPC_TORRENTFILE_H
