// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>

#include "libtremotesf/log.h"
#include "tremotesf_dbus_generated/org.freedesktop.Notifications.h"
#include "tremotesf/desktoputils.h"
#include "tremotesf/rpc/servers.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

namespace tremotesf {
    namespace {
        class FreedesktopNotificationsController : public NotificationsController {
        public:
            explicit FreedesktopNotificationsController(QSystemTrayIcon* trayIcon, QObject* parent = nullptr)
                : NotificationsController(trayIcon, parent) {
                mInterface.setTimeout(desktoputils::defaultDbusTimeout);
                QObject::connect(&mInterface, &OrgFreedesktopNotificationsInterface::ActionInvoked, this, [this] {
                    logInfo("FreedesktopNotificationsController: notification clicked");
                    emit notificationClicked();
                });
            }

            void showNotification(const QString& title, const QString& message) override {
                logInfo(
                    "FreedesktopNotificationsController: executing org.freedesktop.Notifications.Notify() D-Bus call"
                );
                const auto call = new QDBusPendingCallWatcher(
                    mInterface.Notify(
                        TREMOTESF_APP_NAME ""_l1,
                        0,
                        TREMOTESF_APP_ID ""_l1,
                        title,
                        message,
                        {"default"_l1, qApp->translate("tremotesf", "Show Tremotesf")},
                        {{"desktop-entry"_l1, TREMOTESF_APP_ID ""_l1},
                         {"x-kde-origin-name"_l1, Servers::instance()->currentServerName()}},
                        -1
                    ),
                    this
                );
                const auto onFinished = [=] {
                    if (!call->isError()) {
                        logInfo("FreedesktopNotificationsController: executed org.freedesktop.Notifications.Notify() "
                                "D-Bus call");
                    } else {
                        logWarning(
                            "FreedesktopNotificationsController: org.freedesktop.Notifications.Notify() D-Bus call "
                            "failed: {}",
                            call->error()
                        );
                        fallbackToSystemTrayIcon(title, message);
                    }
                };
                if (call->isFinished()) {
                    onFinished();
                } else {
                    QObject::connect(call, &QDBusPendingCallWatcher::finished, this, [=] {
                        onFinished();
                        call->deleteLater();
                    });
                }
            }

        private:
            OrgFreedesktopNotificationsInterface mInterface{
                "org.freedesktop.Notifications"_l1, "/org/freedesktop/Notifications"_l1, QDBusConnection::sessionBus()};
        };
    }

    NotificationsController* NotificationsController::createInstance(QSystemTrayIcon* trayIcon, QObject* parent) {
        return new FreedesktopNotificationsController(trayIcon, parent);
    }
}
