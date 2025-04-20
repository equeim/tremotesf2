// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "addressutils.h"

#include <QHostAddress>
#include <QNetworkInterface>

#include "log/log.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QHostAddress)

namespace tremotesf {
    bool isLocalIpAddress(const QHostAddress& ipAddress) {
        info().log("isLocalIpAddress() called for: ipAddress = {}", ipAddress);
        if (ipAddress.isLoopback()) {
            info().log("isLocalIpAddress: address is loopback, return true");
            return true;
        }
        const auto addresses = QNetworkInterface::allAddresses();
        info().log("isLocalIpAddress: this machine's IP addresses:");
        for (const auto& address : addresses) {
            info().log("isLocalIpAddress: - {}", address);
        }
        if (addresses.contains(ipAddress)) {
            info().log("isLocalIpAddress: address is this machine's IP address, return true");
            return true;
        }
        info().log("isLocalIpAddress: address is not this machine's IP address, return false");
        return false;
    }

    [[nodiscard]] std::optional<bool> isLocalIpAddress(const QString& address) {
        info().log("isLocalIpAddress() called for: address = {}", address);
        const QHostAddress ipAddress(address);
        if (ipAddress.isNull()) {
            info().log("isLocalIpAddress: address is not an IP address");
            return std::nullopt;
        }
        info().log("isLocalIpAddress: address is an IP address");
        return isLocalIpAddress(ipAddress);
    }
}
