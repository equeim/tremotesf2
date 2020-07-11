 /*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
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

#ifndef TREMOTESF_IPCSERVER_DBUS_SERVICE_H
#define TREMOTESF_IPCSERVER_DBUS_SERVICE_H

#include <QObject>

class IpcDbusServiceAdaptor;
class IpcDbusServiceDeprecatedAdaptor;

namespace tremotesf
{
    class IpcServerDbus;

    class IpcDbusService final : public QObject
    {
        Q_OBJECT
    public:
        static const QLatin1String desktopStartupIdField;
        static const QLatin1String torrentHashField;

        explicit IpcDbusService(IpcServerDbus* ipcServer);

    private:
        /*
         * org.freedesktop.Application methods
         */
        friend IpcDbusServiceAdaptor;
        void Activate(const QVariantMap& platform_data);
        void Open(const QStringList& uris, const QVariantMap& platform_data);
        void ActivateAction(const QString& action_name, const QVariantList& parameter, const QVariantMap& platform_data);

        /*
         * org.equeim.Tremotesf methods, deprecated
         */
        friend IpcDbusServiceDeprecatedAdaptor;
        void ActivateWindow();
        void SetArguments(const QStringList& files, const QStringList& urls);
        void OpenTorrentPropertiesPage(const QString& torrentHash);

        IpcServerDbus* mIpcServer;
    };
}

#endif // TREMOTESF_IPCSERVER_DBUS_SERVICE_H
