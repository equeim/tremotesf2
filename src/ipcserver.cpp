/*
 * Tremotesf
 * Copyright (C) 2015-2017 Alexey Rochev <equeim@gmail.com>
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

#include "ipcserver.h"

#include <QDebug>
#include <QFileInfo>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QUrl>
#include <QVariantMap>

namespace tremotesf
{
    namespace
    {
        const QString name(QLatin1String("tremotesf"));

        void sendMessage(const QByteArray& message)
        {
            QLocalSocket socket;
            socket.connectToServer(name);
            if (socket.waitForConnected(1000)) {
                socket.write(message);
                socket.waitForBytesWritten(1000);
            }
        }
    }

    IpcServer::IpcServer(QObject* parent)
        : QLocalServer(parent)
    {
        if (!listen(name)) {
            if (serverError() == QAbstractSocket::AddressInUseError) {
                qWarning() << "Removing dead socket";
                removeServer(name);
                if (!listen(name)) {
                    qWarning() << errorString();
                }
            } else {
                qWarning() << errorString();
            }
        }

        QObject::connect(this, &IpcServer::newConnection, this, [=]() {
            QLocalSocket* socket = nextPendingConnection();
            socket->waitForReadyRead();
            if (socket->state() == QLocalSocket::ConnectedState) {
                const QByteArray message(socket->readAll());
                if (message == "ping") {
                    qWarning() << "Pinged";
                    emit pinged();
                } else {
                    const QVariantMap map(QJsonDocument::fromJson(message).toVariant().toMap());
                    if (map.contains("files") && map.contains(QLatin1String("urls"))) {
                        const QStringList files(map.value(QLatin1String("files")).toStringList());
                        if (!files.isEmpty()) {
                            emit filesReceived(files);
                        }
                        const QStringList urls(map.value(QLatin1String("urls")).toStringList());
                        if (!urls.isEmpty()) {
                            emit urlsReceived(urls);
                        }
                    } else {
                        qWarning() << "Unknown message";
                    }
                }
            }
        });
    }

    bool IpcServer::tryToConnect()
    {
        QLocalSocket socket;
        socket.connectToServer(name);
        return socket.waitForConnected(1000);
    }

    ArgumentsParseResult IpcServer::parseArguments(const QStringList& arguments)
    {
        ArgumentsParseResult result;
        for (const QString& torrent : arguments) {
            const QFileInfo info(torrent);
            if (info.isFile()) {
                result.files.append(info.absoluteFilePath());
            } else {
                const QUrl url(torrent);
                if (url.isLocalFile()) {
                    result.files.append(url.path());
                } else {
                    result.urls.append(torrent);
                }
            }
        }
        return result;
    }

    void IpcServer::ping()
    {
        qWarning() << "Pinging";
        sendMessage("ping");
    }

    void IpcServer::sendArguments(const QStringList& arguments)
    {
        qWarning() << "Sending arguments";
        const ArgumentsParseResult result(parseArguments(arguments));
        sendMessage(QJsonDocument::fromVariant(QVariantMap{{QLatin1String("files"), result.files},
                                                           {QLatin1String("urls"), result.urls}})
                        .toJson());
    }
}
