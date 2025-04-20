// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

namespace tremotesf {
    NotificationsController*
    NotificationsController::createInstance(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent) {
        return new NotificationsController(trayIcon, rpc, parent);
    }
}
