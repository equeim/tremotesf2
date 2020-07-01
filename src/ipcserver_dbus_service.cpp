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

#include "ipcserver_dbus.h"
#include "ipcserver_dbus_service_adaptor.h"

namespace tremotesf
{
    IpcDbusService::IpcDbusService(IpcServerDbus* ipcServer)
        : QObject(ipcServer),
          mIpcServer(ipcServer)
    {
        new IpcDbusServiceAdaptor(this);

        auto connection(QDBusConnection::sessionBus());
        if (connection.registerService(IpcServerDbus::serviceName())) {
            qDebug("Registered D-Bus service");
            if (connection.registerObject(IpcServerDbus::objectPath(), this)) {
                qDebug("Registered D-Bus object");
            } else {
                qWarning() << "Failed to register D-Bus object" << connection.lastError();
            }
        } else {
            qWarning() << "Failed toregister D-Bus service" << connection.lastError();
        }
    }

    void IpcDbusService::ActivateWindow()
    {
        qDebug("Window activation requested");
        emit mIpcServer->windowActivationRequested();
    }

    void IpcDbusService::SetArguments(const QStringList& files, const QStringList& urls)
    {
        qDebug() << "Received arguments files =" << files << "urls =" << urls;
        if (!files.isEmpty()) {
            emit mIpcServer->filesReceived(files);
        }
        if (!urls.isEmpty()) {
            emit mIpcServer->urlsReceived(urls);
        }
    }

    void IpcDbusService::OpenTorrentPropertiesPage(const QString& torrentHash)
    {
#ifdef TREMOTESF_SAILFISHOS
        qDebug() << "Torrent properties page requested, torrent hash =" << torrentHash;
        emit mIpcServer->torrentPropertiesPageRequested(torrentHash);
#else
        qWarning("OpenTorrentPropertiesPage() is supported only on Sailfish OS");
#endif
    }
}
