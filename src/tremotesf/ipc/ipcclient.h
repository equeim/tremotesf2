// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_IPCCLIENT_H
#define TREMOTESF_IPCCLIENT_H

#include <memory>
#include <QStringList>

namespace tremotesf {
    class IpcClient {

    public:
        static std::unique_ptr<IpcClient> createInstance();

        virtual ~IpcClient() = default;
        virtual bool isConnected() const = 0;
        virtual void activateWindow() = 0;
        virtual void addTorrents(const QStringList& files, const QStringList& urls) = 0;

    protected:
        IpcClient() = default;
    };
}

#endif // TREMOTESF_IPCCLIENT_H
