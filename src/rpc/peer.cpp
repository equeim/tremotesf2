// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "peer.h"

#include <QJsonObject>

#include "stdutils.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    Peer::Peer(QString&& address, const QJsonObject& peerJson)
        : address(std::move(address)), client(peerJson.value("clientName"_L1).toString()) {
        update(peerJson);
    }

    bool Peer::update(const QJsonObject& peerJson) {
        bool changed = false;
        setChanged(downloadSpeed, peerJson.value("rateToClient"_L1).toInteger(), changed);
        setChanged(uploadSpeed, peerJson.value("rateToPeer"_L1).toInteger(), changed);
        setChanged(progress, peerJson.value("progress"_L1).toDouble(), changed);
        setChanged(flags, peerJson.value("flagStr"_L1).toString(), changed);
        return changed;
    }
}
