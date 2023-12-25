// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipcserver_dbus_service.h"

#include <QUrl>

#include "log/log.h"
#include "tremotesf_dbus_generated/ipc/org.freedesktop.Application.adaptor.h"
#include "ipcserver_dbus.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QStringList)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariantMap)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariantList)

namespace tremotesf {
    namespace {
        WindowActivationToken platformDataToWindowActivationToken(const QVariantMap& platformData) {
            WindowActivationToken token{};
            if (const auto startupId = platformData.value(IpcDbusService::desktopStartupIdField).toByteArray();
                !startupId.isEmpty()) {
                token.x11StartupNotificationId = startupId;
            }
            if (const auto activationToken = platformData.value(IpcDbusService::xdgActivationTokenField).toByteArray();
                !activationToken.isEmpty()) {
                token.waylandXdfActivationToken = activationToken;
            }
            return token;
        }
    }

    IpcDbusService::IpcDbusService(IpcServerDbus* ipcServer, QObject* parent) : QObject(parent), mIpcServer(ipcServer) {
        new OrgFreedesktopApplicationAdaptor(this);

        auto connection(QDBusConnection::sessionBus());
        if (connection.registerService(IpcServerDbus::serviceName)) {
            logInfo("Registered D-Bus service");
            if (connection.registerObject(IpcServerDbus::objectPath, this)) {
                logInfo("Registered D-Bus object");
            } else {
                logWarning("Failed to register D-Bus object: {}", connection.lastError());
            }
        } else {
            logWarning("Failed to register D-Bus service", connection.lastError());
        }
    }

    /*
     * org.freedesktop.Application methods
     */
    void IpcDbusService::Activate(const QVariantMap& platform_data) {
        logInfo("Window activation requested, platform_data = {}", platform_data);
        emit mIpcServer->windowActivationRequested(
            platform_data.value(torrentHashField).toString(),
            platformDataToWindowActivationToken(platform_data)
        );
    }

    void IpcDbusService::Open(const QStringList& uris, const QVariantMap& platform_data) {
        logInfo("Torrents adding requested, uris = {}, platform_data = {}", uris, platform_data);
        QStringList files;
        QStringList urls;
        for (const QUrl& url : QUrl::fromStringList(uris)) {
            if (url.isValid()) {
                if (url.isLocalFile()) {
                    files.push_back(url.toLocalFile());
                } else {
                    urls.push_back(url.toString());
                }
            }
        }
        emit mIpcServer->torrentsAddingRequested(files, urls, platformDataToWindowActivationToken(platform_data));
    }

    void IpcDbusService::ActivateAction(
        const QString& action_name, const QVariantList& parameter, const QVariantMap& platform_data
    ) {
        logInfo(
            "Action activated, action_name = {}, parameter = {}, platform_data = {}",
            action_name,
            parameter,
            platform_data
        );
    }
}
