// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

namespace tremotesf {
    NotificationsController* NotificationsController::createInstance(QSystemTrayIcon* trayIcon, QObject* parent) {
        return new NotificationsController(trayIcon, parent);
    }
}
