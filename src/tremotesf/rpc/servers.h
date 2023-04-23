// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_SERVERS_H
#define TREMOTESF_SERVERS_H

#include <span>
#include <vector>

#include <QObject>

#include "libtremotesf/rpc.h"

class QSettings;

namespace tremotesf {
    struct MountedDirectory {
        QString localPath{};
        QString remotePath{};

        static QVariant toVariant(std::span<const MountedDirectory> dirs);
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

    struct Server {
        QString name{};
        libtremotesf::ConnectionConfiguration connectionConfiguration{};
        std::vector<MountedDirectory> mountedDirectories{};
        LastTorrents lastTorrents{};
        QStringList lastDownloadDirectories{};
        QString lastDownloadDirectory{};
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
        QString
        fromLocalToRemoteDirectory(const QString& localPath, const libtremotesf::ServerSettings* serverSettings);
        QString
        fromRemoteToLocalDirectory(const QString& remotePath, const libtremotesf::ServerSettings* serverSettings);

        LastTorrents currentServerLastTorrents() const;
        void saveCurrentServerLastTorrents(const libtremotesf::Rpc* rpc);

        QStringList currentServerLastDownloadDirectories(const libtremotesf::ServerSettings* serverSettings) const;
        void setCurrentServerLastDownloadDirectories(const QStringList& directories);

        QString currentServerLastDownloadDirectory(const libtremotesf::ServerSettings* serverSettings) const;
        void setCurrentServerLastDownloadDirectory(const QString& directory);

        void setServer(
            const QString& oldName,
            const QString& name,
            const QString& address,
            int port,
            const QString& apiPath,

            libtremotesf::ConnectionConfiguration::ProxyType proxyType,
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
