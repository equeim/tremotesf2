#include "notificationscontroller.h"

namespace tremotesf {
    NotificationsController* NotificationsController::createInstance(QSystemTrayIcon* trayIcon, QObject* parent) {
        return new NotificationsController(trayIcon, parent);
    }
}
