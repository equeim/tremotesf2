// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationscontroller.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QUuid>

#include "coroutines/dbus.h"
#include "coroutines/scope.h"
#include "desktoputils.h"
#include "log/log.h"
#include "tremotesf_dbus_generated/org.freedesktop.portal.Notification.h"
#include "tremotesf_dbus_generated/org.freedesktop.Notifications.h"
#include "rpc/servers.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

namespace tremotesf {
    namespace {
        constexpr auto flatpakEnvVariable = "FLATPAK_ID";

        class PortalNotificationsController final : public NotificationsController {
        public:
            explicit PortalNotificationsController(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent = nullptr)
                : NotificationsController(trayIcon, rpc, parent) {
                qDBusRegisterMetaType<QPair<QString, QDBusVariant>>();
                mPortalInterface.setTimeout(desktoputils::defaultDbusTimeout);
            }

        protected:
            void showNotification(const QString& title, const QString& message) override {
                mCoroutineScope.launch(showNotificationViaPortal(title, message));
            }

        private:
            Coroutine<> showNotificationViaPortal(QString title, QString message) {
                info().log("PortalNotificationsController: executing "
                           "org.freedesktop.portal.Notification.AddNotification() D-Bus call");
                const auto reply = mPortalInterface.AddNotification(
                    QUuid::createUuid().toString(QUuid::WithoutBraces),
                    {
                        {"title"_l1, title},
                        {"body"_l1, message},
                        {"icon"_l1,
                         QVariant::fromValue(QPair<QString, QDBusVariant>(
                             "themed"_l1,
                             QDBusVariant(QStringList{TREMOTESF_APP_ID ""_l1})
                         ))},
                        {"default-action"_l1, "app.default"_l1},
                        // We just need to put something here
                        {"default-action-target", true},
                    }
                );
                co_await reply;
                if (!reply.isError()) {
                    info().log("PortalNotificationsController: executed "
                               "org.freedesktop.portal.Notification.AddNotification() D-Bus call");
                    co_return;
                }
                warning().log(
                    "PortalNotificationsController: org.freedesktop.portal.Notification.AddNotification() D-Bus "
                    "call failed: "
                    "{}",
                    reply.error()
                );
                fallbackToSystemTrayIcon(title, message);
            }

            OrgFreedesktopPortalNotificationInterface mPortalInterface{
                "org.freedesktop.portal.Desktop"_l1, "/org/freedesktop/portal/desktop"_l1, QDBusConnection::sessionBus()
            };
            CoroutineScope mCoroutineScope{};
        };

        class LegacyFreedesktopNotificationsController final : public NotificationsController {
        public:
            explicit LegacyFreedesktopNotificationsController(
                QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent = nullptr
            )
                : NotificationsController(trayIcon, rpc, parent) {
                qDBusRegisterMetaType<QPair<QString, QDBusVariant>>();
                mLegacyInterface.setTimeout(desktoputils::defaultDbusTimeout);
                QObject::connect(&mLegacyInterface, &OrgFreedesktopNotificationsInterface::ActionInvoked, this, [this] {
                    info().log("LegacyFreedesktopNotificationsController: notification clicked");
                    emit notificationClicked(mActivationToken);
                    mActivationToken.reset();
                });
                QObject::connect(
                    &mLegacyInterface,
                    &OrgFreedesktopNotificationsInterface::ActivationToken,
                    this,
                    [this](uint, const QString& activationToken) {
                        info().log("LegacyFreedesktopNotificationsController: activationToken = {}", activationToken);
                        mActivationToken = activationToken.toUtf8();
                    }
                );
            }

        protected:
            void showNotification(const QString& title, const QString& message) override {
                mCoroutineScope.launch(showNotificationViaLegacyInterface(title, message));
            }

        private:
            Coroutine<> showNotificationViaLegacyInterface(QString title, QString message) {
                info().log("LegacyFreedesktopNotificationsController: executing org.freedesktop.Notifications.Notify() "
                           "D-Bus call");
                const auto reply = mLegacyInterface.Notify(
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
                co_await reply;
                if (!reply.isError()) {
                    info().log("LegacyFreedesktopNotificationsController: executed "
                               "org.freedesktop.Notifications.Notify() D-Bus call");
                    co_return;
                }
                warning().log(
                    "LegacyFreedesktopNotificationsController: org.freedesktop.Notifications.Notify() D-Bus call "
                    "failed: "
                    "{}",
                    reply.error()
                );
                fallbackToSystemTrayIcon(title, message);
            }

            OrgFreedesktopNotificationsInterface mLegacyInterface{
                "org.freedesktop.Notifications"_l1, "/org/freedesktop/Notifications"_l1, QDBusConnection::sessionBus()
            };
            std::optional<QByteArray> mActivationToken{};
            CoroutineScope mCoroutineScope{};
        };
    }

    NotificationsController*
    NotificationsController::createInstance(QSystemTrayIcon* trayIcon, const Rpc* rpc, QObject* parent) {
        // We can technically use Notification portal outside of Flatpak,
        // however it only works properly if we were launched in a very special way through systemd
        // (https://systemd.io/DESKTOP_ENVIRONMENTS, "XDG standardization for applications" section)
        // If we were launched from command line or any other way, then notification won't be properly associated with the app
        // Just use legacy interface instead, it still works
        if (qEnvironmentVariableIsSet(flatpakEnvVariable)) {
            return new PortalNotificationsController(trayIcon, rpc, parent);
        }
        return new LegacyFreedesktopNotificationsController(trayIcon, rpc, parent);
    }
}
