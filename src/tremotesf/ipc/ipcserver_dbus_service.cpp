  /*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ipcserver_dbus_service.h"

#include <QUrl>

#include "libtremotesf/log.h"
#include "tremotesf_dbus_generated/ipc/org.freedesktop.Application.adaptor.h"
#include "ipcserver_dbus.h"

SPECIALIZE_FORMATTER_FOR_QDEBUG(QDBusError)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QStringList)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariantMap)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariantList)

namespace tremotesf
{
    const QLatin1String IpcDbusService::desktopStartupIdField("desktop-startup-id");
    const QLatin1String IpcDbusService::torrentHashField("torrent-hash");

    IpcDbusService::IpcDbusService(IpcServerDbus* ipcServer, QObject *parent)
        : QObject(parent),
          mIpcServer(ipcServer)
    {
        new OrgFreedesktopApplicationAdaptor(this);

        auto connection(QDBusConnection::sessionBus());
        if (connection.registerService(IpcServerDbus::serviceName())) {
            logInfo("Registered D-Bus service");
            if (connection.registerObject(IpcServerDbus::objectPath(), this)) {
                logInfo("Registered D-Bus object");
            } else {
                logWarning("Failed to register D-Bus object: {}", connection.lastError());
            }
        } else {
            logWarning("Failed toregister D-Bus service", connection.lastError());
        }
    }

    /*
     * org.freedesktop.Application methods
     */
    void IpcDbusService::Activate(const QVariantMap& platform_data)
    {
        logInfo("Window activation requested, platform_data = {}", platform_data);
        emit mIpcServer->windowActivationRequested(platform_data.value(torrentHashField).toString(), platform_data.value(desktopStartupIdField).toByteArray());
    }

    void IpcDbusService::Open(const QStringList& uris, const QVariantMap& platform_data)
    {
        logInfo("Torrents adding requested, uris = {}, platform_data = {}", uris, platform_data);
        QStringList files;
        QStringList urls;
        for (const QUrl& url : QUrl::fromStringList(uris)) {
            if (url.isValid()) {
                if (url.isLocalFile()) {
                    files.push_back(url.path());
                } else {
                    urls.push_back(url.toString());
                }
            }
        }
        emit mIpcServer->torrentsAddingRequested(files, urls, platform_data.value(desktopStartupIdField).toByteArray());
    }

    void IpcDbusService::ActivateAction(const QString& action_name, const QVariantList& parameter, const QVariantMap& platform_data)
    {
        logInfo("Action activated, action_name = {}, parameter = {}, platform_data = {}", action_name, parameter, platform_data);
    }
}
