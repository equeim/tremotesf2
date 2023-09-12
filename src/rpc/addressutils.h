// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_RPC_ADDRESSUTILS_H
#define TREMOTESF_RPC_ADDRESSUTILS_H

#include <optional>

class QHostAddress;
class QString;

namespace tremotesf {
    [[nodiscard]] bool isLocalIpAddress(const QHostAddress& ipAddress);
    [[nodiscard]] std::optional<bool> isLocalIpAddress(const QString& address);
}

#endif // TREMOTESF_RPC_ADDRESSUTILS_H
