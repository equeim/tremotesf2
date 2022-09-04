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

#ifndef TREMOTESF_SERVERS_H
#define TREMOTESF_SERVERS_H

#include <vector>
#include <utility>

#include <QObject>

#include "libtremotesf/rpc.h"

class QSettings;

namespace tremotesf
{
    struct Server : libtremotesf::Server
    {
        Server() = default;

        Server(const QString& name,
               const QString& address,
               int port,
               const QString& apiPath,

               ProxyType proxyType,
               const QString& proxyHostname,
               int proxyPort,
               const QString& proxyUser,
               const QString& proxyPassword,

               bool https,
               bool selfSignedCertificateEnabled,
               const QByteArray& selfSignedCertificate,
               bool clientCertificateEnabled,
               const QByteArray& clientCertificate,

               bool authentication,
               const QString& username,
               const QString& password,

               int updateInterval,
               int timeout,

               bool autoReconnectEnabled,
               int autoReconnectInterval,

               const QVariantMap& mountedDirectories,
               const QVariant& lastTorrents,
               const QVariant& addTorrentDialogDirectories);

        QVariantMap mountedDirectories;
        QVariant lastTorrents;
        QVariant addTorrentDialogDirectories;
    };

    struct LastTorrents
    {
        struct Torrent
        {
            QString hashString;
            bool finished;
        };

        bool saved = false;
        std::vector<Torrent> torrents;
    };

    class Servers : public QObject
    {
        Q_OBJECT
    public:
        static Servers* instance();

        static void migrate();

        bool hasServers() const;
        std::vector<Server> servers();

        Server currentServer() const;
        QString currentServerName() const;
        QString currentServerAddress();
        void setCurrentServer(const QString& name);

        bool currentServerHasMountedDirectories() const;
        bool isUnderCurrentServerMountedDirectory(const QString& path) const;
        QString firstLocalDirectory() const;
        QString fromLocalToRemoteDirectory(const QString& path);
        QString fromRemoteToLocalDirectory(const QString& path);

        LastTorrents currentServerLastTorrents() const;
        void saveCurrentServerLastTorrents(const libtremotesf::Rpc* rpc);

        QStringList currentServerAddTorrentDialogDirectories() const;
        void setCurrentServerAddTorrentDialogDirectories(const QStringList& directories);

        void setServer(const QString& oldName,
                                   const QString& name,
                                   const QString& address,
                                   int port,
                                   const QString& apiPath,

                                   libtremotesf::Server::ProxyType proxyType,
                                   const QString& proxyHostname,
                                   int proxyPort,
                                   const QString& proxyUser,
                                   const QString& proxyPassword,

                                   bool https,
                                   bool selfSignedCertificateEnabled,
                                   const QByteArray& selfSignedCertificate,
                                   bool clientCertificateEnabled,
                                   const QByteArray& clientCertificate,

                                   bool authentication,
                                   const QString& username,
                                   const QString& password,

                                   int updateInterval,
                                   int timeout,

                                   bool autoReconnectEnabled,
                                   int autoReconnectInterval,

                                   const QVariantMap& mountedDirectories);

        void removeServer(const QString& name);

        void saveServers(const std::vector<Server>& servers, const QString& current);

    private:
        explicit Servers(QObject* parent = nullptr);
        Server getServer(const QString& name) const;
        void updateMountedDirectories(const QVariantMap& directories);
        void updateMountedDirectories();

        QSettings* mSettings;
        std::vector<std::pair<QString, QString>> mCurrentServerMountedDirectories;
    signals:
        void currentServerChanged();
        void hasServersChanged();
    };
}

#endif // TREMOTESF_SERVERS_H
