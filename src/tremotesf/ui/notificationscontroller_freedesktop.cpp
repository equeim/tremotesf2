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
                logInfo("FreedesktopNotificationsController: executing org.freedesktop.Notifications.Notify() D-Bus call");
                const auto call = new QDBusPendingCallWatcher(mInterface.Notify(
                    QLatin1String(TREMOTESF_APP_NAME),
                    0,
                    QLatin1String(TREMOTESF_APP_ID),
                    title,
                    message,
                    {QLatin1String("default"), qApp->translate("tremotesf", "Show Tremotesf")},
                    {{QLatin1String("desktop-entry"), QLatin1String(TREMOTESF_APP_ID)},
                     {QLatin1String("x-kde-origin-name"), Servers::instance()->currentServerName()}},
                    -1
                ), this);
                const auto onFinished = [=]{
                    if (!call->isError()) {
                        logInfo("FreedesktopNotificationsController: executed org.freedesktop.Notifications.Notify() D-Bus call");
                    } else {
                        logWarning("FreedesktopNotificationsController: org.freedesktop.Notifications.Notify() D-Bus call failed: {}", call->error());
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
                QLatin1String("org.freedesktop.Notifications"),
                QLatin1String("/org/freedesktop/Notifications"),
                QDBusConnection::sessionBus()
            };
        };
    }

    NotificationsController* NotificationsController::createInstance(QSystemTrayIcon* trayIcon, QObject* parent) {
        return new FreedesktopNotificationsController(trayIcon, parent);
    }
}
