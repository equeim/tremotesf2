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

#ifndef LIBTREMOTESF_PEER_H
#define LIBTREMOTESF_PEER_H

#include <QString>

#include "stdutils.h"

class QJsonObject;

namespace libtremotesf
{
    struct Peer
    {
        static const QJsonKeyString addressKey;

        explicit Peer(QString&& address, const QJsonObject& peerJson);
        bool update(const QJsonObject& peerJson);

        bool operator==(const Peer& other) const {
            return address == other.address;
        }

        QString address;
        QString client;
        long long downloadSpeed;
        long long uploadSpeed;
        double progress;
        QString flags;
    };
}

#endif // LIBTREMOTESF_PEER_H
