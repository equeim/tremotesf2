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

#include <QDebug>
#include <QUrl>

#include "ipcserver_dbus.h"
#include "ipcserver_dbus_service_adaptor.h"
#include "ipcserver_dbus_service_deprecated_adaptor.h"

namespace tremotesf
{
    const QLatin1String IpcDbusService::desktopStartupIdField("desktop-startup-id");
    const QLatin1String IpcDbusService::torrentHashField("torrent-hash");

    IpcDbusService::IpcDbusService(IpcServerDbus* ipcServer)
        : QObject(ipcServer),
          mIpcServer(ipcServer)
    {
        new IpcDbusServiceAdaptor(this);
        new IpcDbusServiceDeprecatedAdaptor(this);

        auto connection(QDBusConnection::sessionBus());
        if (connection.registerService(IpcServerDbus::serviceName())) {
            qInfo("Registered D-Bus service");
            if (connection.registerObject(IpcServerDbus::objectPath(), this)) {
                qInfo("Registered D-Bus object");
            } else {
                qWarning() << "Failed to register D-Bus object" << connection.lastError();
            }
        } else {
            qWarning() << "Failed toregister D-Bus service" << connection.lastError();
        }
    }

    /*
     * org.freedesktop.Application methods
     */
    void IpcDbusService::Activate(const QVariantMap& platform_data)
    {
        qInfo().nospace() << "Window activation requested, platform_data=" << platform_data;
        emit mIpcServer->windowActivationRequested(platform_data.value(torrentHashField).toString(), platform_data.value(desktopStartupIdField).toByteArray());
    }

    void IpcDbusService::Open(const QStringList& uris, const QVariantMap& platform_data)
    {
        qInfo().nospace() << "Torrents adding requested, uris=" << uris << ", platform_data=" << platform_data;
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
        qInfo().nospace() << "Action activated, action_name=" << action_name << ", parameter=" << parameter << ", platform_data=" << platform_data;
    }

    /*
     * org.equeim.Tremotesf methods, deprecated
     */
    void IpcDbusService::ActivateWindow()
    {
        qInfo("Window activation requested");
        emit mIpcServer->windowActivationRequested({}, {});
    }

    void IpcDbusService::SetArguments(const QStringList& files, const QStringList& urls)
    {
        qInfo() << "Received arguments files =" << files << "urls =" << urls;
        emit mIpcServer->torrentsAddingRequested(files, urls, {});
    }

    void IpcDbusService::OpenTorrentPropertiesPage(const QString& torrentHash)
    {
#ifdef TREMOTESF_SAILFISHOS
        qInfo() << "Torrent properties page requested, torrent hash =" << torrentHash;
        emit mIpcServer->torrentPropertiesPageRequested(torrentHash);
#else
        Q_UNUSED(torrentHash)
        qWarning("OpenTorrentPropertiesPage() is supported only on Sailfish OS");
#endif
    }
}
