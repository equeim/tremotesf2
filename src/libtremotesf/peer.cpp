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

#include "peer.h"

#include <QJsonObject>

namespace libtremotesf
{
    const QLatin1String Peer::addressKey("address");

    Peer::Peer(QString&& address, const QJsonObject& peerJson)
        : address(std::move(address)),
          client(peerJson.value(QLatin1String("clientName")).toString())
    {
        update(peerJson);
    }

    bool Peer::update(const QJsonObject& peerJson)
    {
        bool changed = false;
        setChanged(downloadSpeed, static_cast<long long>(peerJson.value(QLatin1String("rateToClient")).toDouble()), changed);
        setChanged(uploadSpeed, static_cast<long long>(peerJson.value(QLatin1String("rateToPeer")).toDouble()), changed);
        setChanged(progress, peerJson.value(QLatin1String("progress")).toDouble(), changed);
        setChanged(flags, peerJson.value(QLatin1String("flagStr")).toString(), changed);
        return changed;
    }
}
