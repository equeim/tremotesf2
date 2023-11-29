// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipcclient.h"

#include <QtGlobal>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QUrl>

#include "log/log.h"
#include "tremotesf_dbus_generated/ipc/org.freedesktop.Application.h"
#include "ipcserver_dbus.h"
#include "ipcserver_dbus_service.h"
#include "unixhelpers.h"

namespace tremotesf {
    namespace {
        constexpr auto desktopStartupIdEnvVariable = "DESKTOP_STARTUP_ID";

        inline bool waitForReply(QDBusPendingReply<> pending) {
            pending.waitForFinished();
            const auto reply(pending.reply());
            if (reply.type() != QDBusMessage::ReplyMessage) {
                logWarning("D-Bus method call failed: {}", reply.errorMessage());
                return false;
            }
            return true;
        }
    }

    class IpcClientDbus final : public IpcClient {
    public:
        IpcClientDbus() = default;

        [[nodiscard]] bool isConnected() const override { return mInterface.isValid(); }

        void activateWindow() override {
            logInfo("Requesting window activation");
            if (mInterface.isValid()) {
                waitForReply(mInterface.Activate(getPlatformData()));
            }
        }

        void addTorrents(const QStringList& files, const QStringList& urls) override {
            logInfo("Requesting torrents adding");
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
        static inline QVariantMap getPlatformData() {
            QVariantMap data{};
            if (qEnvironmentVariableIsSet(desktopStartupIdEnvVariable)) {
                const auto startupId = qgetenv(desktopStartupIdEnvVariable);
                logInfo("{} is '{}'", desktopStartupIdEnvVariable, startupId);
                data.insert(IpcDbusService::desktopStartupIdField, startupId);
            } else {
                logInfo("{} is not set", desktopStartupIdEnvVariable);
            }
            if (qEnvironmentVariableIsSet(xdgActivationTokenEnvVariable)) {
                const auto activationToken = qgetenv(xdgActivationTokenEnvVariable);
                logInfo("{} is '{}'", xdgActivationTokenEnvVariable, activationToken);
                data.insert(IpcDbusService::xdgActivationTokenField, qgetenv(xdgActivationTokenEnvVariable));
            } else {
                logInfo("{} is not set", xdgActivationTokenEnvVariable);
            }
            return data;
        }

        OrgFreedesktopApplicationInterface mInterface{
            IpcServerDbus::serviceName, IpcServerDbus::objectPath, QDBusConnection::sessionBus()
        };
    };

    std::unique_ptr<IpcClient> IpcClient::createInstance() { return std::make_unique<IpcClientDbus>(); }
}
