// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "addressutils.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QString>

#include <set>
#include <fmt/ranges.h>

#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QHostAddress)

namespace tremotesf {
    bool isLocalIpAddress(const QHostAddress& ipAddress) {
        logInfo("isLocalIpAddress() called for: ipAddress = {}", ipAddress);
        if (ipAddress.isLoopback()) {
            logInfo("isLocalIpAddress: address is loopback, return true");
            return true;
        }
        const auto addresses = QNetworkInterface::allAddresses();
        logInfo("isLocalIpAddress: this machine's IP addresses:");
        for (const auto& address : addresses) {
            logInfo("isLocalIpAddress: - {}", address);
        }
        if (QNetworkInterface::allAddresses().contains(ipAddress)) {
            logInfo("isLocalIpAddress: address is this machine's IP address, return true");
            return true;
        }
        logInfo("isLocalIpAddress: address is not this machine's IP address, return false");
        return false;
    }

    [[nodiscard]] std::optional<bool> isLocalIpAddress(const QString& address) {
        logInfo("isLocalIpAddress() called for: address = {}", address);
        const QHostAddress ipAddress(address);
        if (ipAddress.isNull()) {
            logInfo("isLocalIpAddress: address is not an IP address");
            return std::nullopt;
        }
        logInfo("isLocalIpAddress: address is an IP address");
        return isLocalIpAddress(ipAddress);
    }
}
