// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipcserver_dbus.h"

namespace tremotesf {
    IpcServer* IpcServer::createInstance(QObject* parent) { return new IpcServerDbus(parent); }
}
