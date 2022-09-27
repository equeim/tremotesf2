// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    public:
        static inline QLatin1String serviceName() { return QLatin1String("org.equeim.Tremotesf"); };
        static inline QLatin1String objectPath() { return QLatin1String("/org/equeim/Tremotesf"); };
        static inline QLatin1String interfaceName() { return QLatin1String("org.freedesktop.Application"); };

        inline explicit IpcServerDbus(QObject* parent = nullptr) : IpcServer(parent) {};

    private:
        IpcDbusService mDbusService{this, this};
    };
}

#endif // TREMOTESF_IPCSERVER_DBUS_H
