// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_IPCSERVER_DBUS_SERVICE_H
#define TREMOTESF_IPCSERVER_DBUS_SERVICE_H

#include <QObject>

class OrgFreedesktopApplicationAdaptor;

namespace tremotesf {
    class IpcServerDbus;

    class IpcDbusService final : public QObject {
        Q_OBJECT

    public:
        static constexpr auto desktopStartupIdField = QLatin1String("desktop-startup-id");
        static constexpr auto xdgActivationTokenField = QLatin1String("activation-token");

        IpcDbusService(IpcServerDbus* ipcServer, QObject* parent = nullptr);

    private:
        /*
         * org.freedesktop.Application methods
         */
        friend OrgFreedesktopApplicationAdaptor;
        void Activate(const QVariantMap& platform_data);
        void Open(const QStringList& uris, const QVariantMap& platform_data);
        void
        ActivateAction(const QString& action_name, const QVariantList& parameter, const QVariantMap& platform_data);

        IpcServerDbus* mIpcServer;
    };
}

#endif // TREMOTESF_IPCSERVER_DBUS_SERVICE_H
