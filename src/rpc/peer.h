// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_PEER_H
#define TREMOTESF_RPC_PEER_H

#include <QString>

class QJsonObject;

namespace tremotesf {
    struct Peer {
        static constexpr auto addressKey = QLatin1String("address");

        explicit Peer(QString&& address, const QJsonObject& peerJson);
        bool update(const QJsonObject& peerJson);

        bool operator==(const Peer& other) const = default;

        QString address{};
        QString client{};
        qint64 downloadSpeed{};
        qint64 uploadSpeed{};
        double progress{};
        QString flags{};
    };
}

#endif // TREMOTESF_RPC_PEER_H
