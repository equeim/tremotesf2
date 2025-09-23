// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
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

using namespace Qt::StringLiterals;

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)

namespace tremotesf {
    namespace {
        constexpr auto flatpakEnvVariable = "FLATPAK_ID";

        class PortalNotificationsController final : public NotificationsController {
            Q_OBJECT
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
                info().log(
                    "PortalNotificationsController: executing "
                    "org.freedesktop.portal.Notification.AddNotification() D-Bus call"
                );
                const auto reply = mPortalInterface.AddNotification(
                    QUuid::createUuid().toString(QUuid::WithoutBraces),
                    {
                        {"title"_L1, title},
                        {"body"_L1, message},
                        {"icon"_L1,
                         QVariant::fromValue(
                             QPair<QString, QDBusVariant>(
                                 "themed"_L1,
                                 QDBusVariant(QStringList{TREMOTESF_APP_ID ""_L1})
                             )
                         )},
                        {"default-action"_L1, "app.default"_L1},
                        // We just need to put something here
                        {"default-action-target", true},
                    }
                );
                co_await reply;
                if (!reply.isError()) {
                    info().log(
                        "PortalNotificationsController: executed "
                        "org.freedesktop.portal.Notification.AddNotification() D-Bus call"
                    );
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
                "org.freedesktop.portal.Desktop"_L1, "/org/freedesktop/portal/desktop"_L1, QDBusConnection::sessionBus()
            };
            CoroutineScope mCoroutineScope{};
        };

        class LegacyFreedesktopNotificationsController final : public NotificationsController {
            Q_OBJECT
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
                info().log(
                    "LegacyFreedesktopNotificationsController: executing org.freedesktop.Notifications.Notify() "
                    "D-Bus call"
                );
                const auto reply = mLegacyInterface.Notify(
                    TREMOTESF_APP_NAME ""_L1,
                    0,
                    TREMOTESF_APP_ID ""_L1,
                    title,
                    message,
                    //: Button on notification
                    {"default"_L1, qApp->translate("tremotesf", "Show Tremotesf")},
                    {{"desktop-entry"_L1, TREMOTESF_APP_ID ""_L1},
                     {"x-kde-origin-name"_L1, Servers::instance()->currentServerName()}},
                    -1
                );
                co_await reply;
                if (!reply.isError()) {
                    info().log(
                        "LegacyFreedesktopNotificationsController: executed "
                        "org.freedesktop.Notifications.Notify() D-Bus call"
                    );
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
                "org.freedesktop.Notifications"_L1, "/org/freedesktop/Notifications"_L1, QDBusConnection::sessionBus()
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

#include "notificationscontroller_freedesktop.moc"
