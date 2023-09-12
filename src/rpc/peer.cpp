// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "peer.h"

#include <QJsonObject>

#include "jsonutils.h"
#include "literals.h"
#include "stdutils.h"

namespace tremotesf {
    using namespace impl;

    Peer::Peer(QString&& address, const QJsonObject& peerJson)
        : address(std::move(address)), client(peerJson.value("clientName"_l1).toString()) {
        update(peerJson);
    }

    bool Peer::update(const QJsonObject& peerJson) {
        bool changed = false;
        setChanged(downloadSpeed, toInt64(peerJson.value("rateToClient"_l1)), changed);
        setChanged(uploadSpeed, toInt64(peerJson.value("rateToPeer"_l1)), changed);
        setChanged(progress, peerJson.value("progress"_l1).toDouble(), changed);
        setChanged(flags, peerJson.value("flagStr"_l1).toString(), changed);
        return changed;
    }
}
