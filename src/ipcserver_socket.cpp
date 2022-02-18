/*
 * Tremotesf
 * Copyright (C) 2015-2018 Alexey Rochev <equeim@gmail.com>
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

#include "ipcserver_socket.h"

#include <QDebug>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include "utils.h"
#endif

namespace tremotesf
{
    IpcServerSocket::IpcServerSocket(QObject* parent)
        : IpcServer(parent)
    {
        auto server = new QLocalServer(this);

        const QString name(socketName());

        if (!server->listen(name)) {
            if (server->serverError() == QAbstractSocket::AddressInUseError) {
                // We already tried to connect to it, removing
                qWarning("Removing dead socket");
                if (server->removeServer(name)) {
                    if (!server->listen(name)) {
                        qWarning() << "Failed to create socket," << server->errorString();
                    }
                } else {
                    qWarning() << "Failed to remove socket," << server->errorString();
                }
            } else {
                qWarning() << "Failed to create socket," << server->errorString();
            }
        }

        if (!server->isListening()) {
            return;
        }

        QObject::connect(server, &QLocalServer::newConnection, this, [=]() {
            QLocalSocket* socket = server->nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
            QTimer::singleShot(30000, socket, &QLocalSocket::disconnectFromServer);
            QObject::connect(socket, &QLocalSocket::readyRead, this, [=]() {
                const QByteArray message(socket->readAll());
                qInfo() << "read" << message;
                if (message == "ping") {
                    qInfo("Window activation requested");
                    emit windowActivationRequested({}, {});
                } else {
                    const QVariantMap arguments(QJsonDocument::fromJson(message).toVariant().toMap());
                    qInfo() << "Arguments received" << arguments;
                    const QStringList files(arguments.value(QLatin1String("files")).toStringList());
                    const QStringList urls(arguments.value(QLatin1String("urls")).toStringList());
                    emit torrentsAddingRequested(files, urls, {});
                }
            });
        });
    }

    QString IpcServerSocket::socketName()
    {
            QString name(QLatin1String("tremotesf"));
#ifdef Q_OS_WIN
            try {
                DWORD sessionId{};
                Utils::callWinApiFunctionWithLastError([&] { return ProcessIdToSessionId(GetCurrentProcessId(), &sessionId); });
                name += '-';
                name += QString::number(sessionId);
            } catch (const std::exception& e) {
                qWarning() << "ProcessIdToSessionId falied: " << e.what();
            }
#endif
            return name;
    }

    IpcServer* IpcServer::createInstance(QObject* parent)
    {
        return new IpcServerSocket(parent);
    }
}
