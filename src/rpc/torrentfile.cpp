// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentfile.h"

#include <QJsonObject>
#include <QStringList>

#include "jsonutils.h"
#include "literals.h"
#include "stdutils.h"

namespace tremotesf {
    using namespace impl;
    namespace {
        constexpr auto priorityMapper = EnumMapper(std::array{
            EnumMapping(TorrentFile::Priority::Low, -1),
            EnumMapping(TorrentFile::Priority::Normal, 0),
            EnumMapping(TorrentFile::Priority::High, 1)
        });
    }

    TorrentFile::TorrentFile(int id, const QJsonObject& fileMap, const QJsonObject& fileStatsMap)
        : id(id), size(toInt64(fileMap.value("length"_l1))) {
        auto p = fileMap.value("name"_l1).toString().split(QLatin1Char('/'), Qt::SkipEmptyParts);
        path.reserve(static_cast<size_t>(p.size()));
        for (QString& part : p) {
            path.push_back(std::move(part));
        }
        update(fileStatsMap);
    }

    bool TorrentFile::update(const QJsonObject& fileStatsMap) {
        bool changed = false;

        setChanged(completedSize, toInt64(fileStatsMap.value("bytesCompleted"_l1)), changed);
        constexpr auto priorityKey = "priority"_l1;
        setChanged(priority, priorityMapper.fromJsonValue(fileStatsMap.value(priorityKey), priorityKey), changed);
        setChanged(wanted, fileStatsMap.value("wanted"_l1).toBool(), changed);

        return changed;
    }
}
