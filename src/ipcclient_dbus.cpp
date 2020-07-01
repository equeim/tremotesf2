/*
 * Tremotesf
 * Copyright (C) 2015-2019 Alexey Rochev <equeim@gmail.com>
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

#include "ipcclient.h"

#include <QDBusInterface>
#include <QDebug>

#include "ipcclient_dbus_interface.h"
#include "ipcserver_dbus.h"

namespace tremotesf
{
    namespace
    {
        inline void waitForReply(QDBusPendingReply<>&& pending)
        {
            pending.waitForFinished();
            const auto reply(pending.reply());
            if (reply.type() != QDBusMessage::ReplyMessage) {
                qWarning() << "D-Bus method call failed, error string:" << reply.errorMessage();
            }
        }
    }

    class IpcClientDbus final : public IpcClient
    {
    public:
        bool isConnected() const override
        {
            return mInterface.isValid();
        }

        void activateWindow() override
        {
            qInfo("Requesting window activation");
            waitForReply(mInterface.ActivateWindow());
        }

        void sendArguments(const QStringList& files, const QStringList& urls) override
        {
            qInfo("Sending arguments");
            waitForReply(mInterface.SetArguments(files, urls));
        }

    private:
        IpcDbusInterface mInterface{IpcServerDbus::serviceName(), IpcServerDbus::objectPath(), QDBusConnection::sessionBus()};
    };

    std::unique_ptr<IpcClient> IpcClient::createInstance()
    {
        return std::make_unique<IpcClientDbus>();
    }
}
