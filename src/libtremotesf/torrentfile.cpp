/*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#include "torrentfile.h"

#include <QJsonObject>
#include <QStringList>

#include "stdutils.h"

namespace libtremotesf
{
    TorrentFile::TorrentFile(int id, const QJsonObject& fileMap, const QJsonObject& fileStatsMap)
        : id(id), size(static_cast<long long>(fileMap.value(QLatin1String("length")).toDouble()))
    {
        auto p = fileMap
                .value(QLatin1String("name"))
                .toString()
                .split(QLatin1Char('/'), Qt::SkipEmptyParts);
        path.reserve(static_cast<size_t>(p.size()));
        for (QString& part : p) {
            path.push_back(std::move(part));
        }
        update(fileStatsMap);
    }

    bool TorrentFile::update(const QJsonObject& fileStatsMap)
    {
        bool changed = false;

        setChanged(completedSize, static_cast<long long>(fileStatsMap.value(QLatin1String("bytesCompleted")).toDouble()), changed);
        setChanged(priority, [&] {
            switch (int p = fileStatsMap.value(QLatin1String("priority")).toInt()) {
            case TorrentFile::LowPriority:
            case TorrentFile::NormalPriority:
            case TorrentFile::HighPriority:
                return static_cast<TorrentFile::Priority>(p);
            default:
                return TorrentFile::NormalPriority;
            }
        }(), changed);
        setChanged(wanted, fileStatsMap.value(QLatin1String("wanted")).toBool(), changed);

        return changed;
    }
}
