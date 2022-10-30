// SPDX-FileCopyrightText: 2015-2022 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERS_H
#define TREMOTESF_SERVERS_H

#include <vector>

#include <QObject>

#include "libtremotesf/rpc.h"

class QSettings;

namespace tremotesf {
    struct MountedDirectory {
        QString localPath{};
        QString remotePath{};

        static QVariant toVariant(const std::vector<MountedDirectory>& dirs);
        static std::vector<MountedDirectory> fromVariant(const QVariant& var);
    };

    struct LastTorrents {
        struct Torrent {
            QString hashString{};
            bool finished{};
        };

        bool saved{};
        std::vector<Torrent> torrents{};

        QVariant toVariant() const;
        static LastTorrents fromVariant(const QVariant& var);
    };

    struct Server : libtremotesf::Server {
        Server() = default;

        Server(
            const QString& name,
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

            const std::vector<MountedDirectory>& mountedDirectories,
            const LastTorrents& lastTorrents,
            const QStringList& addTorrentDialogDirectories
        );

        std::vector<MountedDirectory> mountedDirectories{};
        LastTorrents lastTorrents{};
        QStringList addTorrentDialogDirectories{};
    };

    class Servers : public QObject {
        Q_OBJECT
    public:
        static Servers* instance();

        bool hasServers() const;
        std::vector<Server> servers();

        Server currentServer() const;
        QString currentServerName() const;
        QString currentServerAddress();
        void setCurrentServer(const QString& name);

        bool currentServerHasMountedDirectories() const;
        bool isUnderCurrentServerMountedDirectory(const QString& path) const;
        QString fromLocalToRemoteDirectory(const QString& localPath);
        QString fromRemoteToLocalDirectory(const QString& remotePath);

        LastTorrents currentServerLastTorrents() const;
        void saveCurrentServerLastTorrents(const libtremotesf::Rpc* rpc);

        QStringList currentServerAddTorrentDialogDirectories() const;
        void setCurrentServerAddTorrentDialogDirectories(const QStringList& directories);

        void setServer(
            const QString& oldName,
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

            const std::vector<MountedDirectory>& mountedDirectories
        );

        void removeServer(const QString& name);

        void saveServers(const std::vector<Server>& servers, const QString& current);

    private:
        explicit Servers(QObject* parent = nullptr);
        Server getServer(const QString& name) const;
        void updateMountedDirectories();

        QSettings* mSettings{};
        std::vector<MountedDirectory> mCurrentServerMountedDirectories{};
    signals:
        void currentServerChanged();
        void hasServersChanged();
    };
}

#endif // TREMOTESF_SERVERS_H
