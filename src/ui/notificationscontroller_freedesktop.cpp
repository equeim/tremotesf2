// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>

#include "log/log.h"
#include "tremotesf_dbus_generated/org.freedesktop.Notifications.h"
#include "desktoputils.h"
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
                info().log(
                    "FreedesktopNotificationsController: executing org.freedesktop.Notifications.Notify() D-Bus call"
                );
                const auto call = new QDBusPendingCallWatcher(
                    mInterface.Notify(
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
                    ),
                    this
                );
                const auto onFinished = [=, this] {
                    if (!call->isError()) {
                        info().log("FreedesktopNotificationsController: executed org.freedesktop.Notifications.Notify() "
                                "D-Bus call");
                    } else {
                        warning().log(
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
                "org.freedesktop.Notifications"_l1, "/org/freedesktop/Notifications"_l1, QDBusConnection::sessionBus()
            };
        };
    }

    NotificationsController*
    NotificationsController::createInstance(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent) {
        return new FreedesktopNotificationsController(trayIcon, rpc, parent);
    }
}
