// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipcserver_dbus_service.h"

#include <QUrl>
#include <KWindowSystem>

#include "log/log.h"
#include "tremotesf_dbus_generated/ipc/org.freedesktop.Application.adaptor.h"
#include "ipcserver_dbus.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QStringList)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariantMap)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariantList)

namespace tremotesf {
    namespace {
        std::optional<QByteArray> platformDataToWindowActivationToken(const QVariantMap& platformData) {
            QLatin1String key{};
            switch (KWindowSystem::platform()) {
            case KWindowSystem::Platform::X11: {
                key = IpcDbusService::desktopStartupIdField;
                break;
            }
            case KWindowSystem::Platform::Wayland: {
                key = IpcDbusService::xdgActivationTokenField;
                break;
            }
            default:
                warning().log("Unknown windowing system");
                return {};
            }
            auto token = platformData.value(key).toByteArray();
            if (token.isEmpty()) {
                return {};
            }
            return token;
        }
    }

    IpcDbusService::IpcDbusService(IpcServerDbus* ipcServer, QObject* parent) : QObject(parent), mIpcServer(ipcServer) {
        new OrgFreedesktopApplicationAdaptor(this);

        auto connection(QDBusConnection::sessionBus());
        if (connection.registerService(IpcServerDbus::serviceName)) {
            info().log("Registered D-Bus service");
            if (connection.registerObject(IpcServerDbus::objectPath, this)) {
                info().log("Registered D-Bus object");
            } else {
                warning().log("Failed to register D-Bus object: {}", connection.lastError());
            }
        } else {
            warning().log("Failed to register D-Bus service", connection.lastError());
        }
    }

    /*
     * org.freedesktop.Application methods
     */
    void IpcDbusService::Activate(const QVariantMap& platform_data) {
        info().log("IpcDbusService: window activation requested, platform_data = {}", platform_data);
        emit mIpcServer->windowActivationRequested(
            platform_data.value(torrentHashField).toString(),
            platformDataToWindowActivationToken(platform_data)
        );
    }

    void IpcDbusService::Open(const QStringList& uris, const QVariantMap& platform_data) {
        info().log("IpcDbusService: torrents adding requested, uris = {}, platform_data = {}", uris, platform_data);
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
        info().log(
            "IpcDbusService: action activated, action_name = {}, parameter = {}, platform_data = {}",
            action_name,
            parameter,
            platform_data
        );
        emit mIpcServer->windowActivationRequested({}, platformDataToWindowActivationToken(platform_data));
    }
}
