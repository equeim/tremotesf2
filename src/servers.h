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
               bool https,
               bool selfSignedCertificateEnabled,
               const QByteArray& selfSignedCertificate,
               bool clientCertificateEnabled,
               const QByteArray& clientCertificate,
               bool authentication,
               const QString& username,
               const QString& password,
               int updateInterval,
               int backgroundUpdateInterval,
               int timeout,
               const QVariantMap& mountedDirectories,
               const QVariant& lastTorrents,
               const QVariant& addTorrentDialogDirectories);

        Server(QString&& name,
               QString&& address,
               int port,
               QString&& apiPath,
               bool https,
               bool selfSignedCertificateEnabled,
               QByteArray&& selfSignedCertificate,
               bool clientCertificateEnabled,
               QByteArray&& clientCertificate,
               bool authentication,
               QString&& username,
               QString&& password,
               int updateInterval,
               int backgroundUpdateInterval,
               int timeout,
               QVariantMap&& mountedDirectories,
               QVariant&& lastTorrents,
               QVariant&& addTorrentDialogDirectories);

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
        Q_PROPERTY(bool hasServers READ hasServers NOTIFY hasServersChanged)
        Q_PROPERTY(libtremotesf::Server currentServer READ currentServer NOTIFY currentServerChanged)
        Q_PROPERTY(QString currentServerName READ currentServerName NOTIFY currentServerChanged)
        Q_PROPERTY(QString currentServerAddress READ currentServerAddress NOTIFY currentServerChanged)
        Q_PROPERTY(bool currentServerHasMountedDirectories READ currentServerHasMountedDirectories NOTIFY currentServerChanged)
        Q_PROPERTY(QString firstLocalDirectory READ firstLocalDirectory NOTIFY currentServerChanged)
    public:
        static Servers* instance();

        static void migrate();

        bool hasServers() const;
        std::vector<Server> servers();

        Server currentServer() const;
        QString currentServerName() const;
        QString currentServerAddress();
        Q_INVOKABLE void setCurrentServer(const QString& name);

        bool currentServerHasMountedDirectories() const;
        Q_INVOKABLE bool isUnderCurrentServerMountedDirectory(const QString& path) const;
        QString firstLocalDirectory() const;
        Q_INVOKABLE QString fromLocalToRemoteDirectory(const QString& path);
        Q_INVOKABLE QString fromRemoteToLocalDirectory(const QString& path);

        LastTorrents currentServerLastTorrents() const;
        Q_INVOKABLE void saveCurrentServerLastTorrents(const libtremotesf::Rpc* rpc);

        QStringList currentServerAddTorrentDialogDirectories() const;
        void setCurrentServerAddTorrentDialogDirectories(const QStringList& directories);

        Q_INVOKABLE void setServer(const QString& oldName,
                                   const QString& name,
                                   const QString& address,
                                   int port,
                                   const QString& apiPath,
                                   bool https,
                                   bool selfSignedCertificateEnabled,
                                   const QByteArray& selfSignedCertificate,
                                   bool clientCertificateEnabled,
                                   const QByteArray& clientCertificate,
                                   bool authentication,
                                   const QString& username,
                                   const QString& password,
                                   int updateInterval,
                                   int backgroundUpdateInterval,
                                   int timeout,
                                   const QVariantMap& mountedDirectories);

        Q_INVOKABLE void removeServer(const QString& name);

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
