// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_IPCCLIENT_H
#define TREMOTESF_IPCCLIENT_H

#include <memory>
#include <QStringList>

namespace tremotesf
{
    class IpcClient
    {
    public:
        static std::unique_ptr<IpcClient> createInstance();

        IpcClient() = default;

        Q_DISABLE_COPY(IpcClient)

        IpcClient(IpcClient&&) = delete;
        IpcClient& operator=(IpcClient&&) = delete;

        virtual ~IpcClient() = default;
        virtual bool isConnected() const = 0;
        virtual void activateWindow() = 0;
        virtual void addTorrents(const QStringList& files, const QStringList& urls) = 0;
    };
}

#endif // TREMOTESF_IPCCLIENT_H
