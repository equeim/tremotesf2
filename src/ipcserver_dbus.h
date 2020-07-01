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

#ifndef TREMOTESF_IPCSERVER_DBUS_H
#define TREMOTESF_IPCSERVER_DBUS_H

#include <QObject>

#include "ipcserver.h"
#include "ipcserver_dbus_service.h"

namespace tremotesf
{
    class IpcServerDbus final : public IpcServer
    {
        Q_OBJECT
        Q_PROPERTY(QString serviceName READ serviceName CONSTANT)
        Q_PROPERTY(QString objectPath READ objectPath CONSTANT)
        Q_PROPERTY(QString interfaceName READ interfaceName CONSTANT)
    public:
        static inline QLatin1String serviceName() { return QLatin1String("org.equeim.Tremotesf"); };
        static inline QLatin1String objectPath() { return QLatin1String("/org/equeim/Tremotesf"); };
        static inline QLatin1String interfaceName() { return QLatin1String("org.equeim.Tremotesf"); };

        inline explicit IpcServerDbus(QObject* parent = nullptr) : IpcServer(parent) {};

    private:
        IpcDbusService mDbusService{this};

#ifdef TREMOTESF_SAILFISHOS
    signals:
        void torrentPropertiesPageRequested(const QString& torrentHash);
#endif
    };
}

#endif // TREMOTESF_IPCSERVER_DBUS_H
