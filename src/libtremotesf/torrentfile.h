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

#ifndef LIBTREMOTESF_TORRENTFILE_H
#define LIBTREMOTESF_TORRENTFILE_H

#include <vector>

class QString;

//#include <QString>

class QJsonObject;

namespace libtremotesf
{
    struct TorrentFile
    {
        enum Priority
        {
            LowPriority = -1,
            NormalPriority,
            HighPriority
        };

        explicit TorrentFile(int id, const QJsonObject& fileMap, const QJsonObject& fileStatsMap);
        bool update(const QJsonObject& fileStatsMap);

        int id;

        std::vector<QString> path;
        long long size;
        long long completedSize = 0;
        Priority priority = NormalPriority;
        bool wanted = false;
    };
}

#endif // LIBTREMOTESF_TORRENTFILE_H
