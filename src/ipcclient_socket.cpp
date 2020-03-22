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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>

#include "ipcserver.h"

namespace tremotesf
{
    class IpcClient::Private
    {
    public:
        void sendMessage(const QByteArray& message)
        {
            const qint64 written = socket.write(message);
            if (written != message.size()) {
                qWarning() << "Failed to write to socket," << written << "bytes written";
            }
            if (!socket.waitForBytesWritten()) {
                qWarning("Timed out when waiting for bytes written");
            }
        }

        QLocalSocket socket;
    };

    IpcClient::IpcClient() : d(new Private())
    {
        d->socket.connectToServer(IpcServer::socketName());
        d->socket.waitForConnected();
    }

    IpcClient::~IpcClient() = default;

    bool IpcClient::isConnected() const
    {
        return d->socket.state() == QLocalSocket::ConnectedState;
    }

    void IpcClient::activateWindow()
    {
        qInfo("Requesting window activation");
        d->sendMessage("ping");
    }

    void IpcClient::sendArguments(const QStringList& files, const QStringList& urls)
    {
        qInfo("Sending arguments");
        d->sendMessage(QJsonDocument(QJsonObject{{QLatin1String("files"), QJsonArray::fromStringList(files)},
                                                 {QLatin1String("urls"), QJsonArray::fromStringList(urls)}}).toJson(QJsonDocument::Compact));
    }
}
