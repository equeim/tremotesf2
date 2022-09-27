#include "ipcserver_dbus.h"

namespace tremotesf
{
    IpcServer* IpcServer::createInstance(QObject* parent)
    {
        return new IpcServerDbus(parent);
    }
}
