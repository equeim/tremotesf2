// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QPointer>

#include "coroutines/dbus.h"
#include "coroutines/scope.h"
#include "desktoputils.h"
#include "log/log.h"
#include "tremotesf_dbus_generated/org.freedesktop.Notifications.h"
#include "rpc/servers.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

namespace tremotesf {
    namespace {
        class FreedesktopNotificationsController final : public NotificationsController {
        public:
            explicit FreedesktopNotificationsController(
                QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent = nullptr
            )
                : NotificationsController(trayIcon, rpc, parent) {
                mInterface.setTimeout(desktoputils::defaultDbusTimeout);
                QObject::connect(&mInterface, &OrgFreedesktopNotificationsInterface::ActionInvoked, this, [this] {
                    info().log("FreedesktopNotificationsController: notification clicked");
                    emit notificationClicked();
                });
            }

        protected:
            void showNotification(const QString& title, const QString& message) override {
                mCoroutineScope.launch(showNotificationImpl(title, message));
            }

        private:
            Coroutine<> showNotificationImpl(QString title, QString message) {
                info().log(
                    "FreedesktopNotificationsController: executing org.freedesktop.Notifications.Notify() D-Bus call"
                );
                const auto reply = mInterface.Notify(
                    TREMOTESF_APP_NAME ""_l1,
                    0,
                    TREMOTESF_APP_ID ""_l1,
                    title,
                    message,
                    //: Button on notification
                    {"default"_l1, qApp->translate("tremotesf", "Show Tremotesf")},
                    {{"desktop-entry"_l1, TREMOTESF_APP_ID ""_l1},
                     {"x-kde-origin-name"_l1, Servers::instance()->currentServerName()}},
                    -1
                );
                // Split co_await to separate line here to workaround internal compiler error bug in GCC 13
                co_await reply;
                if (!reply.isError()) {
                    info().log(
                        "FreedesktopNotificationsController: executed org.freedesktop.Notifications.Notify() D-Bus call"
                    );
                } else {
                    warning().log(
                        "FreedesktopNotificationsController: org.freedesktop.Notifications.Notify() D-Bus call failed: "
                        "{}",
                        reply.error()
                    );
                    fallbackToSystemTrayIcon(title, message);
                }
            }

            OrgFreedesktopNotificationsInterface mInterface{
                "org.freedesktop.Notifications"_l1, "/org/freedesktop/Notifications"_l1, QDBusConnection::sessionBus()
            };
            CoroutineScope mCoroutineScope{};
        };
    }

    NotificationsController*
    NotificationsController::createInstance(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent) {
        return new FreedesktopNotificationsController(trayIcon, rpc, parent);
    }
}
