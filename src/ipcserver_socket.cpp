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
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace tremotesf
{
    namespace
    {
        QString socketName()
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

        void sendMessage(const QByteArray& message)
        {
            QLocalSocket socket;
            socket.connectToServer(socketName());
            if (socket.waitForConnected()) {
                const auto written = socket.write(message);
                if (written != message.size()) {
                    qWarning() << "Failed to write to socket," << written << "bytes written";
                }
                if (!socket.waitForBytesWritten()) {
                    qWarning() << "Timed out when waiting for bytes written";
                }
                if (!socket.waitForDisconnected()) {
                    qWarning() << "Timed out when waiting for disconnect";
                }
            } else {
                qWarning() << "Failed to connect to socket";
            }
        }
    }

    IpcServer::IpcServer(QObject* parent)
        : QObject(parent)
    {
        auto server = new QLocalServer(this);

        const QString name(socketName());

        if (!server->listen(name)) {
            if (server->serverError() == QAbstractSocket::AddressInUseError) {
                // We alredy tried to connect to it, removing
                qWarning() << "Removing dead socket";
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
            QObject::connect(socket, &QLocalSocket::readyRead, this, [=]() {
                const QByteArray message(socket->readAll());
                socket->disconnectFromServer();
                socket->deleteLater();
                if (message == "ping") {
                    qDebug() << "Window activation requested";
                    emit windowActivationRequested();
                } else {
                    const QJsonArray arguments(QJsonDocument::fromJson(message).array());
                    qDebug() << "Arguments received" << arguments;
                    ArgumentsParseResult result;
                    for (const QJsonValue& argument : arguments) {
                        parseArgument(argument.toString(), result);
                    }
                    if (!result.files.isEmpty()) {
                        emit filesReceived(result.files);
                    }
                    if (!result.urls.isEmpty()) {
                        emit urlsReceived(result.urls);
                    }
                }
            });

            QTimer::singleShot(30000, socket, [=]() {
                socket->disconnectFromServer();
                socket->deleteLater();
            });
        });
    }

    bool IpcServer::tryToConnect()
    {
        QLocalSocket socket;
        socket.connectToServer(socketName());
        return socket.waitForConnected();
    }

    void IpcServer::activateWindow()
    {
        qInfo() << "Requesting window activation";
        sendMessage("ping");
    }

    void IpcServer::sendArguments(const QStringList& arguments)
    {
        qInfo() << "Sending arguments";
        QJsonArray array;
        for (const QString& argument : arguments) {
            array.push_back(argument);
        }
        sendMessage(QJsonDocument(array).toJson());
    }
}
