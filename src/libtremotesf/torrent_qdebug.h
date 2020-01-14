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

#ifndef LIBTREMOTESF_TORRENT_QDEBUG_H
#define LIBTREMOTESF_TORRENT_QDEBUG_H

#include <QDebug>
#include "torrent.h"

inline QDebug operator<<(QDebug debug, const libtremotesf::Torrent& torrent)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Torrent(id=" << torrent.id() << ", name=" << torrent.name() << ")";
    return debug;
}

inline QDebug operator<<(QDebug debug, const libtremotesf::Torrent* torrent)
{
    return operator<<(debug, *torrent);
}

#endif // LIBTREMOTESF_TORRENT_QDEBUG_H
