// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipcclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>

#include "log/log.h"
#include "ipcserver_socket.h"

namespace tremotesf {
    class IpcClientSocket final : public IpcClient {
    public:
        IpcClientSocket() {
            mSocket.connectToServer(IpcServerSocket::socketName());
            mSocket.waitForConnected();
        }

        bool isConnected() const override { return mSocket.state() == QLocalSocket::ConnectedState; }

        void activateWindow() override {
            info().log("Requesting window activation");
            sendMessage(IpcServerSocket::activateWindowMessage);
        }

        void addTorrents(const QStringList& files, const QStringList& urls) override {
            info().log("Requesting torrents adding");
            sendMessage(IpcServerSocket::createAddTorrentsMessage(files, urls));
        }

    private:
        void sendMessage(const QByteArray& message) {
            const qint64 written = mSocket.write(message);
            if (written != message.size()) {
                warning().log("Failed to write to socket ({} bytes written): {}", written, mSocket.errorString());
            }
            waitForBytesWritten();
        }

        void sendMessage(char message) {
            const bool written = mSocket.putChar(message);
            if (!written) {
                warning().log("Failed to write to socket: {}", mSocket.errorString());
            }
            waitForBytesWritten();
        }

        void waitForBytesWritten() {
            if (!mSocket.waitForBytesWritten()) {
                warning().log("Timed out when waiting for bytes written");
            }
        }

        QLocalSocket mSocket;
    };

    std::unique_ptr<IpcClient> IpcClient::createInstance() { return std::make_unique<IpcClientSocket>(); }
}
