// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_IPCSERVER_DBUS_H
#define TREMOTESF_IPCSERVER_DBUS_H

#include <QObject>

#include "ipcserver.h"
#include "ipcserver_dbus_service.h"

#include "literals.h"

namespace tremotesf {
    class IpcServerDbus final : public IpcServer {
        Q_OBJECT

    public:
        static constexpr auto serviceName = "org.equeim.Tremotesf"_l1;
        static constexpr auto objectPath = "/org/equeim/Tremotesf"_l1;
        static constexpr auto interfaceName = "org.freedesktop.Application"_l1;

        inline explicit IpcServerDbus(QObject* parent = nullptr) : IpcServer(parent){};

    private:
        IpcDbusService mDbusService{this, this};
    };
}

#endif // TREMOTESF_IPCSERVER_DBUS_H
