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

#include "ipcserver.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace tremotesf
{
    IpcServer::IpcServer(QObject* parent)
        : QObject(parent)
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
            QObject::connect(socket, &QLocalSocket::disconnected, socket, [=]() {
                qInfo("disconnected");
                socket->deleteLater();
            });
            QTimer::singleShot(30000, socket, &QLocalSocket::disconnectFromServer);
            QObject::connect(socket, &QLocalSocket::readyRead, this, [=]() {
                const QByteArray message(socket->readAll());
                qInfo() << "read" << message;
                if (message == "ping") {
                    qInfo("Window activation requested");
                    emit windowActivationRequested();
                } else {
                    const QVariantMap arguments(QJsonDocument::fromJson(message).toVariant().toMap());
                    qInfo() << "Arguments received" << arguments;
                    const QStringList files(arguments.value(QLatin1String("files")).toStringList());
                    if (!files.isEmpty()) {
                        emit filesReceived(files);
                    }
                    const QStringList urls(arguments.value(QLatin1String("urls")).toStringList());
                    if (!urls.isEmpty()) {
                        emit urlsReceived(urls);
                    }
                }
            });
        });
    }

    QString IpcServer::socketName()
    {
            QString name(QLatin1String("tremotesf"));
#ifdef Q_OS_WIN
            DWORD sessionId = 0;
            ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
            name += '-';
            name += QString::number(sessionId);
#endif
            return name;
    }
}
