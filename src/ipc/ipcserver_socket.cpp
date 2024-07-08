// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipcserver_socket.h"

#include <QCborArray>
#include <QCborMap>
#include <QCborParserError>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include "fileopeneventhandler.h"
#include "literals.h"
#include "log/log.h"

#ifdef Q_OS_WIN
#    include <windows.h>
#    include "windowshelpers.h"
#endif

#ifdef Q_OS_MACOS
#    include "ipc/fileopeneventhandler.h"
#endif

SPECIALIZE_FORMATTER_FOR_Q_ENUM(QCborError::Code)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QCborValue)

namespace fmt {
    template<>
    struct formatter<QCborParserError> : tremotesf::SimpleFormatter {
        fmt::format_context::iterator format(QCborParserError error, fmt::format_context& ctx) const {
            return fmt::format_to(
                ctx.out(),
                "{} (error code {})",
                error.errorString(),
                static_cast<QCborError::Code>(error.error)
            );
        }
    };
}

namespace tremotesf {
    namespace {
        constexpr QStringView keyFiles = u"files";
        constexpr QStringView keyUrls = u"urls";

        QStringList toStringList(const QCborValue& value) {
            QStringList strings{};
            if (!value.isArray()) return strings;
            const auto array = value.toArray();
            strings.reserve(static_cast<QStringList::size_type>(array.size()));
            for (const auto& v : array) {
                if (v.isString()) {
                    strings.push_back(v.toString());
                }
            }
            return strings;
        }
    }

    IpcServerSocket::IpcServerSocket(QObject* parent) : IpcServer(parent) {
        auto* const server = new QLocalServer(this);

        const QString name(socketName());

        if (!server->listen(name)) {
            if (server->serverError() == QAbstractSocket::AddressInUseError) {
                // We already tried to connect to it, removing
                warning().log("Removing dead socket");
                if (QLocalServer::removeServer(name)) {
                    if (!server->listen(name)) {
                        warning().log("Failed to create socket: {}", server->errorString());
                    }
                } else {
                    warning().log("Failed to remove socket: {}", server->errorString());
                }
            } else {
                warning().log("Failed to create socket: {}", server->errorString());
            }
        }

        if (server->isListening()) {
            listenToConnections(server);
        }

#ifdef Q_OS_MACOS
        const auto* const handler = new FileOpenEventHandler(this);
        QObject::connect(
            handler,
            &FileOpenEventHandler::filesOpeningRequested,
            this,
            [this](const QStringList& files, const QStringList& urls) { emit torrentsAddingRequested(files, urls, {}); }
        );
#endif
    }

    QString IpcServerSocket::socketName() {
        QString name("tremotesf"_l1);
#ifdef Q_OS_WIN
        try {
            DWORD sessionId{};
            checkWin32Bool(ProcessIdToSessionId(GetCurrentProcessId(), &sessionId), "ProcessIdToSessionId");
            name += '-';
            name += QString::number(sessionId);
        } catch (const std::system_error& e) {
            warning().logWithException(e, "IpcServerSocket: failed to append session id to socket name");
        }
#endif
        return name;
    }

    IpcServer* IpcServer::createInstance(QObject* parent) { return new IpcServerSocket(parent); }

    QByteArray IpcServerSocket::createAddTorrentsMessage(const QStringList& files, const QStringList& urls) {
        return QCborMap{{keyFiles, QCborArray::fromStringList(files)}, {keyUrls, QCborArray::fromStringList(urls)}}
            .toCborValue()
            .toCbor();
    }

    void IpcServerSocket::listenToConnections(QLocalServer* server) {
        QObject::connect(server, &QLocalServer::newConnection, this, [this, server]() {
            QLocalSocket* socket = server->nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
            QTimer::singleShot(30000, socket, &QLocalSocket::disconnectFromServer);
            QObject::connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
                const QByteArray message(socket->readAll());
                if (message.size() == 1 && message.front() == activateWindowMessage) {
                    info().log("IpcServerSocket: window activation requested");
                    emit windowActivationRequested({});
                } else {
                    QCborParserError error{};
                    const auto cbor = QCborValue::fromCbor(message, &error);
                    if (error.error != QCborError::NoError) {
                        warning().log("IpcServerSocket: failed to parse CBOR message: {}", error);
                        return;
                    }
                    info().log("Arguments received: {}", cbor);
                    const auto map = cbor.toMap();
                    const auto files = toStringList(map[keyFiles]);
                    const auto urls = toStringList(map[keyUrls]);
                    emit torrentsAddingRequested(files, urls, {});
                }
            });
        });
    }
}
