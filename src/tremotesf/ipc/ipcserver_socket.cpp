#include "ipcserver_socket.h"

#include <QCborArray>
#include <QCborMap>
#include <QCborParserError>
#include <QCborValue>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include "libtremotesf/log.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include "tremotesf/windowshelpers.h"
#endif

SPECIALIZE_FORMATTER_FOR_Q_ENUM(QCborError::Code)
SPECIALIZE_FORMATTER_FOR_QDEBUG(QCborValue)

namespace tremotesf
{
    namespace {
        constexpr QStringView keyFiles = u"files";
        constexpr QStringView keyUrls = u"urls";

        QStringList toStringList(const QCborValue& value) {
            QStringList strings{};
            if (!value.isArray()) return strings;
            const auto array = value.toArray();
            strings.reserve(array.size());
            for (const QCborValue& v : array) {
                if (v.isString()) {
                    strings.push_back(v.toString());
                }
            }
            return strings;
        }
    }

    IpcServerSocket::IpcServerSocket(QObject* parent)
        : IpcServer(parent)
    {
        auto server = new QLocalServer(this);

        const QString name(socketName());

        if (!server->listen(name)) {
            if (server->serverError() == QAbstractSocket::AddressInUseError) {
                // We already tried to connect to it, removing
                logWarning("Removing dead socket");
                if (server->removeServer(name)) {
                    if (!server->listen(name)) {
                        logWarning("Failed to create socket: {}", server->errorString());
                    }
                } else {
                    logWarning("Failed to remove socket: {}", server->errorString());
                }
            } else {
                logWarning("Failed to create socket: {}", server->errorString());
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
                if (message.size() == 1 && message.front() == activateWindowMessage) {
                    logInfo("IpcServerSocket: window activation requested");
                    emit windowActivationRequested({}, {});
                } else {
                    QCborParserError error{};
                    const auto cbor = QCborValue::fromCbor(message, &error);
                    if (error.error != QCborError::NoError) {
                        logWarning("IpcServerSocket: failed to parse CBOR message: {} {}", error.error, error.errorString());
                        return;
                    }
                    logInfo("Arguments received: {}", cbor);
                    const auto map = cbor.toMap();
                    const auto files = toStringList(map[keyFiles]);
                    const auto urls = toStringList(map[keyUrls]);
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
                checkWin32Bool(ProcessIdToSessionId(GetCurrentProcessId(), &sessionId), "ProcessIdToSessionId");
                name += '-';
                name += QString::number(sessionId);
            } catch (const std::system_error& e) {
                logWarningWithException(e, "IpcServerSocket: failed to append session id to socket name");
            }
#endif
            return name;
    }

    IpcServer* IpcServer::createInstance(QObject* parent)
    {
        return new IpcServerSocket(parent);
    }

    QByteArray IpcServerSocket::createAddTorrentsMessage(const QStringList& files, const QStringList& urls) {
        return QCborMap{
            {keyFiles, QCborArray::fromStringList(files)},
            {keyUrls, QCborArray::fromStringList(urls)}
        }.toCborValue().toCbor();
    }
}
