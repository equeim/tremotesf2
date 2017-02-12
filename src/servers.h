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

#ifndef TREMOTESF_SERVERS_H
#define TREMOTESF_SERVERS_H

#include <QObject>

class QSettings;

namespace tremotesf
{
    struct Server
    {
        QString name;
        QString address;
        int port;
        QString apiPath;
        bool https;
        QByteArray localCertificate;
        bool authentication;
        QString username;
        QString password;
        int updateInterval;
        int timeout;
    };

    class Servers : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool hasServers READ hasServers NOTIFY hasServersChanged)
        Q_PROPERTY(QString currentServerName READ currentServerName NOTIFY currentServerChanged)
        Q_PROPERTY(QString currentServerAddress READ currentServerAddress NOTIFY currentServerChanged)
    public:
        static Servers* instance();

#ifdef TREMOTESF_SAILFISHOS
        static void migrateFrom0();
#endif
        static void migrateFromAccounts();

        bool hasServers() const;
        QList<Server> servers();

        Server currentServer();
        QString currentServerName() const;
        QString currentServerAddress();
        Q_INVOKABLE void setCurrentServer(const QString& name);

        Q_INVOKABLE void setServer(const QString& oldName,
                                   const QString& name,
                                   const QString& address,
                                   int port,
                                   const QString& apiPath,
                                   bool https,
                                   const QByteArray& localCertificate,
                                   bool authentication,
                                   const QString& username,
                                   const QString& password,
                                   int updateInterval,
                                   int timeout);

        Q_INVOKABLE void removeServer(const QString& name);

        void saveServers(const QList<Server>& servers, const QString& current);

    private:
        explicit Servers(QObject* parent = nullptr);
        Server getServer(const QString& name);

        QSettings* mSettings;
    signals:
        void currentServerChanged();
        void hasServersChanged();
    };
}

#endif // TREMOTESF_SERVERS_H
