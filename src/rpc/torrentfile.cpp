// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfile.h"

#include <QJsonObject>

#include "jsonutils.h"
#include "stdutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    using namespace impl;
    namespace {
        constexpr auto priorityMapper = EnumMapper(
            std::array{
                EnumMapping(TorrentFile::Priority::Low, -1),
                EnumMapping(TorrentFile::Priority::Normal, 0),
                EnumMapping(TorrentFile::Priority::High, 1)
            }
        );
    }

    TorrentFile::TorrentFile(int id, const QJsonObject& fileMap, const QJsonObject& fileStatsMap)
        : id(id), size(fileMap.value("length"_L1).toInteger()) {
        auto p = fileMap.value("name"_L1).toString().split('/', Qt::SkipEmptyParts);
        path.reserve(static_cast<size_t>(p.size()));
        for (QString& part : p) {
            path.push_back(std::move(part));
        }
        update(fileStatsMap);
    }

    bool TorrentFile::update(const QJsonObject& fileStatsMap) {
        bool changed = false;

        setChanged(completedSize, fileStatsMap.value("bytesCompleted"_L1).toInteger(), changed);
        constexpr auto priorityKey = "priority"_L1;
        setChanged(priority, priorityMapper.fromJsonValue(fileStatsMap.value(priorityKey), priorityKey), changed);
        setChanged(wanted, fileStatsMap.value("wanted"_L1).toBool(), changed);

        return changed;
    }
}
