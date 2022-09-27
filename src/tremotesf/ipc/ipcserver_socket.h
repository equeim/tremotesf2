#ifndef TREMOTESF_IPCSERVER_SOCKET_H
#define TREMOTESF_IPCSERVER_SOCKET_H

#include "ipcserver.h"

namespace tremotesf
{
    class IpcServerSocket final : public IpcServer
    {
    public:
        static QString socketName();
        explicit IpcServerSocket(QObject* parent = nullptr);

        static constexpr char activateWindowMessage = '\0';

        static QByteArray createAddTorrentsMessage(const QStringList& files, const QStringList& urls);
    };
}

#endif // TREMOTESF_IPCSERVER_SOCKET_H
