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

#include <QtGlobal>
#include <QUrl>

#include "libtremotesf/println.h"
#include "ipcserver_dbus.h"
#include "ipcclient_dbus_interface.h"
#include "ipcserver_dbus_service.h"

namespace tremotesf
{
    namespace
    {
        inline bool waitForReply(QDBusPendingReply<>&& pending)
        {
            pending.waitForFinished();
            const auto reply(pending.reply());
            if (reply.type() != QDBusMessage::ReplyMessage) {
                printlnWarning("D-Bus method call failed: {}", reply.errorMessage());
                return false;
            }
            return true;
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
            printlnInfo("Requesting window activation");
            if (mInterface.isValid()) {
                waitForReply(mInterface.Activate(getPlatformData()));
            }
        }

        void addTorrents(const QStringList& files, const QStringList& urls) override
        {
            printlnInfo("Requesting torrents adding");
            if (mInterface.isValid()) {
                QStringList uris;
                uris.reserve(files.size() + urls.size());
                for (const QString& filePath : files) {
                    uris.push_back(QUrl::fromLocalFile(filePath).toString());
                }
                uris.append(urls);
                waitForReply(mInterface.Open(uris, getPlatformData()));
            }
        }

    private:
        static inline QVariantMap getPlatformData()
        {
            if (qEnvironmentVariableIsSet("DESKTOP_STARTUP_ID")) {
                return {{IpcDbusService::desktopStartupIdField, qgetenv("DESKTOP_STARTUP_ID")}};
            }
            return {};
        }

        IpcDbusInterface mInterface{IpcServerDbus::serviceName(), IpcServerDbus::objectPath(), QDBusConnection::sessionBus()};
    };

    std::unique_ptr<IpcClient> IpcClient::createInstance()
    {
        return std::make_unique<IpcClientDbus>();
    }
}
