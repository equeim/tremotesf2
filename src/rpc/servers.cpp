// SPDX-FileCopyrightText: 2015-2024 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servers.h"

#include <ranges>

#include <QCoreApplication>
#include <QSettings>
#include <QStringBuilder>

#include "rpc/pathutils.h"
#include "rpc/serversettings.h"
#include "target_os.h"

using namespace Qt::StringLiterals;

namespace tremotesf {
    namespace {
        constexpr QSettings::Format settingsFormat = [] {
            if constexpr (targetOs == TargetOs::Windows) {
                return QSettings::IniFormat;
            } else {
                return QSettings::NativeFormat;
            }
        }();

        constexpr auto fileName = "servers"_L1;

        constexpr auto currentServerKey = "current"_L1;
        constexpr auto addressKey = "address"_L1;
        constexpr auto portKey = "port"_L1;
        constexpr auto apiPathKey = "apiPath"_L1;

        constexpr auto proxyTypeKey = "proxyType"_L1;
        constexpr auto proxyHostnameKey = "proxyHostname"_L1;
        constexpr auto proxyPortKey = "proxyPort"_L1;
        constexpr auto proxyUserKey = "proxyUser"_L1;
        constexpr auto proxyPasswordKey = "proxyPassword"_L1;

        constexpr auto httpsKey = "https"_L1;
        constexpr auto selfSignedCertificateEnabledKey = "selfSignedCertificateEnabled"_L1;
        constexpr auto selfSignedCertificateKey = "selfSignedCertificate"_L1;
        constexpr auto clientCertificateEnabledKey = "clientCertificateEnabled"_L1;
        constexpr auto clientCertificateKey = "clientCertificate"_L1;

        constexpr auto authenticationKey = "authentication"_L1;
        constexpr auto usernameKey = "username"_L1;
        constexpr auto passwordKey = "password"_L1;

        constexpr auto updateIntervalKey = "updateInterval"_L1;
        constexpr auto timeoutKey = "timeout"_L1;

        constexpr auto autoReconnectEnabledKey = "autoReconnectEnabled"_L1;
        constexpr auto autoReconnectIntervalKey = "autoReconnectInterval"_L1;

        constexpr auto mountedDirectoriesKey = "mountedDirectories"_L1;

        constexpr auto lastDownloadDirectoriesKey = "addTorrentDialogDirectories"_L1;
        constexpr auto lastDownloadDirectoryKey = "lastDownloadDirectory"_L1;

        constexpr auto lastTorrentsKey = "lastTorrents"_L1;
        constexpr auto lastTorrentsHashStringKey = "hashString"_L1;
        constexpr auto lastTorrentsFinishedKey = "finished"_L1;

        constexpr auto localCertificateKey = "localCertificate"_L1;

        constexpr auto proxyTypeDefault = "Default"_L1;
        constexpr auto proxyTypeHttp = "HTTP"_L1;
        constexpr auto proxyTypeSocks5 = "SOCKS5"_L1;
        constexpr auto proxyTypeNone = "None"_L1;

        ConnectionConfiguration::ProxyType proxyTypeFromSettings(const QString& value) {
            if (value.isEmpty() || value == proxyTypeDefault) {
                return ConnectionConfiguration::ProxyType::Default;
            }
            if (value == proxyTypeHttp) {
                return ConnectionConfiguration::ProxyType::Http;
            }
            if (value == proxyTypeSocks5) {
                return ConnectionConfiguration::ProxyType::Socks5;
            }
            if (value == proxyTypeNone) {
                return ConnectionConfiguration::ProxyType::None;
            }
            return ConnectionConfiguration::ProxyType::Default;
        }

        QLatin1String proxyTypeToSettings(ConnectionConfiguration::ProxyType type) {
            switch (type) {
            case ConnectionConfiguration::ProxyType::Default:
                return proxyTypeDefault;
            case ConnectionConfiguration::ProxyType::Http:
                return proxyTypeHttp;
            case ConnectionConfiguration::ProxyType::Socks5:
                return proxyTypeSocks5;
            case ConnectionConfiguration::ProxyType::None:
                return proxyTypeNone;
            }
            return proxyTypeDefault;
        }

        bool isPathUnderThisDirectory(QStringView path, QStringView directory) {
            // Path is not under parentDirectory
            if (!path.startsWith(directory)) {
                return false;
            }
            // Path is the same as parentDirectory - that's ok
            if (path.size() == directory.size()) {
                return true;
            }
            // Path ends with segment that's not actually the last segment of parentDirectory, just prefixed by it
            if (!directory.endsWith('/') && path[directory.size()] != '/') {
                return false;
            }
            return true;
        }
    }

    QVariant MountedDirectory::toVariant(std::span<const MountedDirectory> dirs) {
        QVariantMap map{};
        for (const auto& dir : dirs) {
            map.insert(dir.localPath, dir.remotePath);
        }
        return map;
    }

    std::vector<MountedDirectory> MountedDirectory::fromVariant(const QVariant& var) {
        const QVariantMap map = var.toMap();
        std::vector<MountedDirectory> dirs{};
        dirs.reserve(static_cast<size_t>(map.size()));
        for (const auto& [key, value] : map.asKeyValueRange()) {
            dirs.push_back({.localPath = normalizePath(key, localPathOs), .remotePath = value.toString()});
        }
        return dirs;
    }

    QVariant LastTorrents::toVariant() const {
        return torrents
               | std::views::transform([](const LastTorrents::Torrent& torrent) {
                     return QVariant(
                         QVariantMap{
                             {lastTorrentsHashStringKey, torrent.hashString},
                             {lastTorrentsFinishedKey, torrent.finished}
                         }
                     );
                 })
               | std::ranges::to<QVariantList>();
    }

    LastTorrents LastTorrents::fromVariant(const QVariant& var) {
        if (!var.isValid() || var.typeId() != QMetaType::QVariantList) {
            return {};
        }
        return {
            .saved = true,
            .torrents = var.toList()
                        | std::views::transform([](const QVariant& torrentVar) {
                              const QVariantMap map = torrentVar.toMap();
                              return LastTorrents::Torrent{
                                  .hashString = map[lastTorrentsHashStringKey].toString(),
                                  .finished = map[lastTorrentsFinishedKey].toBool()
                              };
                          })
                        | std::ranges::to<std::vector>()
        };
    }

    Servers* Servers::instance() {
        static auto* const instance = new Servers(nullptr, qApp);
        return instance;
    }

    bool Servers::hasServers() const { return !mSettings->childGroups().isEmpty(); }

    std::vector<Server> Servers::servers() {
        std::vector<Server> list;
        const QStringList groups(mSettings->childGroups());
        list.reserve(static_cast<size_t>(groups.size()));
        for (const QString& group : groups) {
            list.push_back(getServer(group));
        }
        return list;
    }

    Server Servers::currentServer() const { return getServer(currentServerName()); }

    QString Servers::currentServerName() const { return mSettings->value(currentServerKey).toString(); }

    QString Servers::currentServerAddress() {
        mSettings->beginGroup(currentServerName());
        QString address(mSettings->value(addressKey).toString());
        mSettings->endGroup();
        return address;
    }

    void Servers::setCurrentServer(const QString& name) {
        if (mSettings->value(currentServerKey) != name) {
            mSettings->setValue(currentServerKey, name);
            updateMountedDirectories();
            emit currentServerChanged();
        }
    }

    bool Servers::currentServerHasMountedDirectories() const { return !mCurrentServerMountedDirectories.empty(); }

    QString Servers::fromLocalToRemoteDirectory(const QString& localPath, PathOs remotePathOs) {
        const auto localPathNormalized = normalizePath(localPath, localPathOs);
        for (const auto& [localDirectory, remoteDirectory] : mCurrentServerMountedDirectories) {
            if (isPathUnderThisDirectory(localPathNormalized, localDirectory)) {
                const auto remoteDirectoryNormalized = normalizePath(remoteDirectory, remotePathOs);
                auto relativePathIndex = localDirectory.size();
                if (localDirectory.endsWith('/')) {
                    relativePathIndex -= 1;
                }
                return normalizePath(
                    remoteDirectoryNormalized + localPathNormalized.mid(relativePathIndex),
                    remotePathOs
                );
            }
        }
        return {};
    }

    QString Servers::fromLocalToRemoteDirectory(const QString& localPath, const ServerSettings* serverSettings) {
        return fromLocalToRemoteDirectory(localPath, serverSettings->data().pathOs);
    }

    QString Servers::fromRemoteToLocalDirectory(const QString& remotePath, PathOs remotePathOs) {
        const auto remotePathNormalized = normalizePath(remotePath, remotePathOs);
        for (const auto& [localDirectory, remoteDirectory] : mCurrentServerMountedDirectories) {
            const auto remoteDirectoryNormalized = normalizePath(remoteDirectory, remotePathOs);
            if (isPathUnderThisDirectory(remotePathNormalized, remoteDirectoryNormalized)) {
                auto relativePathIndex = remoteDirectoryNormalized.size();
                if (remoteDirectoryNormalized.endsWith('/')) {
                    relativePathIndex -= 1;
                }
                return normalizePath(localDirectory + remotePathNormalized.mid(relativePathIndex), localPathOs);
            }
        }
        return {};
    }

    QString Servers::fromRemoteToLocalDirectory(const QString& remotePath, const ServerSettings* serverSettings) {
        return fromRemoteToLocalDirectory(remotePath, serverSettings->data().pathOs);
    }

    LastTorrents Servers::currentServerLastTorrents() const {
        mSettings->beginGroup(currentServerName());
        auto lastTorrents = LastTorrents::fromVariant(mSettings->value(lastTorrentsKey));
        mSettings->endGroup();
        return lastTorrents;
    }

    void Servers::saveCurrentServerLastTorrents(const Rpc* rpc) {
        if (!hasServers()) {
            return;
        }
        mSettings->beginGroup(currentServerName());
        LastTorrents torrents{};
        torrents.torrents = rpc->torrents()
                            | std::views::transform([](const auto& torrent) {
                                  return LastTorrents::Torrent{
                                      .hashString = torrent->data().hashString,
                                      .finished = torrent->data().isFinished()
                                  };
                              })
                            | std::ranges::to<std::vector>();
        mSettings->setValue(lastTorrentsKey, torrents.toVariant());
        mSettings->endGroup();
    }

    QStringList Servers::currentServerLastDownloadDirectories(const ServerSettings* serverSettings) const {
        QStringList directories{};
        mSettings->beginGroup(currentServerName());
        directories = mSettings->value(lastDownloadDirectoriesKey).toStringList()
                      | std::views::transform([serverSettings](const QString& dir) {
                            return normalizePath(dir, serverSettings->data().pathOs);
                        })
                      | std::ranges::to<QStringList>();
        mSettings->endGroup();
        return directories;
    }

    void Servers::setCurrentServerLastDownloadDirectories(const QStringList& directories) {
        mSettings->beginGroup(currentServerName());
        mSettings->setValue(lastDownloadDirectoriesKey, directories);
        mSettings->endGroup();
    }

    QString Servers::currentServerLastDownloadDirectory(const ServerSettings* serverSettings) const {
        QString directory{};
        mSettings->beginGroup(currentServerName());
        directory = normalizePath(mSettings->value(lastDownloadDirectoryKey).toString(), serverSettings->data().pathOs);
        mSettings->endGroup();
        return directory;
    }

    void Servers::setCurrentServerLastDownloadDirectory(const QString& directory) {
        mSettings->beginGroup(currentServerName());
        mSettings->setValue(lastDownloadDirectoryKey, directory);
        mSettings->endGroup();
    }

    void Servers::setServer(
        const QString& oldName,
        const QString& name,
        const QString& address,
        int port,
        const QString& apiPath,

        ConnectionConfiguration::ProxyType proxyType,
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
    ) {
        bool currentChanged = false;
        const QString current(currentServerName());
        if (oldName == current) {
            if (name != oldName) {
                mSettings->setValue(currentServerKey, name);
            }
            currentChanged = true;
        } else if (name == current) {
            currentChanged = true;
        }

        QStringList lastDownloadDirectories{};
        QString lastDownloadDirectory{};
        if (!oldName.isEmpty() && name != oldName) {
            lastDownloadDirectories = mSettings->value(oldName % '/' % lastDownloadDirectoriesKey).toStringList();
            lastDownloadDirectory = mSettings->value(oldName % '/' % lastDownloadDirectoryKey).toString();

            mSettings->remove(oldName);
        }

        mSettings->beginGroup(name);

        mSettings->setValue(addressKey, address);
        mSettings->setValue(portKey, port);
        mSettings->setValue(apiPathKey, apiPath);

        mSettings->setValue(proxyTypeKey, proxyTypeToSettings(proxyType));
        mSettings->setValue(proxyHostnameKey, proxyHostname);
        mSettings->setValue(proxyPortKey, proxyPort);
        mSettings->setValue(proxyUserKey, proxyUser);
        mSettings->setValue(proxyPasswordKey, proxyPassword);

        mSettings->setValue(httpsKey, https);
        mSettings->setValue(selfSignedCertificateEnabledKey, selfSignedCertificateEnabled);
        mSettings->setValue(selfSignedCertificateKey, selfSignedCertificate);
        mSettings->setValue(clientCertificateEnabledKey, clientCertificateEnabled);
        mSettings->setValue(clientCertificateKey, clientCertificate);

        mSettings->setValue(authenticationKey, authentication);
        mSettings->setValue(usernameKey, username);
        mSettings->setValue(passwordKey, password);

        mSettings->setValue(updateIntervalKey, updateInterval);
        mSettings->setValue(timeoutKey, timeout);

        mSettings->setValue(autoReconnectEnabledKey, autoReconnectEnabled);
        mSettings->setValue(autoReconnectEnabledKey, autoReconnectInterval);

        mSettings->setValue(mountedDirectoriesKey, MountedDirectory::toVariant(mountedDirectories));
        mSettings->setValue(lastDownloadDirectoriesKey, lastDownloadDirectories);
        mSettings->setValue(lastDownloadDirectoryKey, lastDownloadDirectory);

        mSettings->endGroup();

        if (currentChanged) {
            mCurrentServerMountedDirectories = mountedDirectories;
            emit currentServerChanged();
        }

        if (oldName.isEmpty() && mSettings->childGroups().size() == 1) {
            emit hasServersChanged();
        }
    }

    void Servers::saveServers(const std::vector<Server>& servers, const QString& current) {
        const bool hadServers = hasServers();
        mSettings->clear();
        if (!current.isEmpty()) {
            mSettings->setValue(currentServerKey, current);
        }
        for (const Server& server : servers) {
            mSettings->beginGroup(server.name);

            mSettings->setValue(addressKey, server.connectionConfiguration.address);
            mSettings->setValue(portKey, server.connectionConfiguration.port);
            mSettings->setValue(apiPathKey, server.connectionConfiguration.apiPath);

            mSettings->setValue(proxyTypeKey, proxyTypeToSettings(server.connectionConfiguration.proxyType));
            mSettings->setValue(proxyHostnameKey, server.connectionConfiguration.proxyHostname);
            mSettings->setValue(proxyPortKey, server.connectionConfiguration.proxyPort);
            mSettings->setValue(proxyUserKey, server.connectionConfiguration.proxyUser);
            mSettings->setValue(proxyPasswordKey, server.connectionConfiguration.proxyPassword);

            mSettings->setValue(httpsKey, server.connectionConfiguration.https);
            mSettings->setValue(
                selfSignedCertificateEnabledKey,
                server.connectionConfiguration.selfSignedCertificateEnabled
            );
            mSettings->setValue(selfSignedCertificateKey, server.connectionConfiguration.selfSignedCertificate);
            mSettings->setValue(clientCertificateEnabledKey, server.connectionConfiguration.clientCertificateEnabled);
            mSettings->setValue(clientCertificateKey, server.connectionConfiguration.clientCertificate);

            mSettings->setValue(authenticationKey, server.connectionConfiguration.authentication);
            mSettings->setValue(usernameKey, server.connectionConfiguration.username);
            mSettings->setValue(passwordKey, server.connectionConfiguration.password);

            mSettings->setValue(updateIntervalKey, server.connectionConfiguration.updateInterval);
            mSettings->setValue(timeoutKey, server.connectionConfiguration.timeout);

            mSettings->setValue(autoReconnectEnabledKey, server.connectionConfiguration.autoReconnectEnabled);
            mSettings->setValue(autoReconnectIntervalKey, server.connectionConfiguration.autoReconnectInterval);

            mSettings->setValue(mountedDirectoriesKey, MountedDirectory::toVariant(server.mountedDirectories));
            mSettings->setValue(lastTorrentsKey, server.lastTorrents.toVariant());
            mSettings->setValue(lastDownloadDirectoriesKey, server.lastDownloadDirectories);
            mSettings->setValue(lastDownloadDirectoryKey, server.lastDownloadDirectory);

            mSettings->endGroup();
        }
        updateMountedDirectories();
        emit currentServerChanged();
        if (hasServers() != hadServers) {
            emit hasServersChanged();
        }
    }

    Servers::Servers(QSettings* settings, QObject* parent)
        : QObject(parent),
          mSettings(
              settings ? settings
                       : new QSettings(settingsFormat, QSettings::UserScope, qApp->organizationName(), fileName, this)
          ) {
        mSettings->setFallbacksEnabled(false);
        if (hasServers()) {
            bool foundCurrent = false;
            const QString current(currentServerName());
            if (!current.isEmpty()) {
                const QStringList groups(mSettings->childGroups());
                for (const QString& group : groups) {
                    if (group == current) {
                        foundCurrent = true;
                        break;
                    }
                }
            }
            if (!foundCurrent) {
                mSettings->setValue(currentServerKey, mSettings->childGroups().constFirst());
            }
        } else {
            mSettings->remove(currentServerKey);
        }

        const QStringList groups(mSettings->childGroups());
        for (const QString& group : groups) {
            mSettings->beginGroup(group);
            if (mSettings->contains(localCertificateKey)) {
                const QByteArray localCertificate(mSettings->value(localCertificateKey).toByteArray());
                if (!localCertificate.isEmpty()) {
                    mSettings->setValue(clientCertificateEnabledKey, true);
                    mSettings->setValue(clientCertificateKey, localCertificate);
                }
                mSettings->remove(localCertificateKey);
            }
            mSettings->endGroup();
        }

        updateMountedDirectories();
    }

    Server Servers::getServer(const QString& name) const {
        mSettings->beginGroup(name);
        Server server{
            mSettings->group(),
            ConnectionConfiguration{
                mSettings->value(addressKey).toString(),
                mSettings->value(portKey).toInt(),
                mSettings->value(apiPathKey).toString(),

                proxyTypeFromSettings(mSettings->value(proxyTypeKey).toString()),
                mSettings->value(proxyHostnameKey).toString(),
                mSettings->value(proxyPortKey).toInt(),
                mSettings->value(proxyUserKey).toString(),
                mSettings->value(proxyPasswordKey).toString(),

                mSettings->value(httpsKey, false).toBool(),
                mSettings->value(selfSignedCertificateEnabledKey, false).toBool(),
                mSettings->value(selfSignedCertificateKey).toByteArray(),
                mSettings->value(clientCertificateEnabledKey, false).toBool(),
                mSettings->value(clientCertificateKey).toByteArray(),

                mSettings->value(authenticationKey, false).toBool(),
                mSettings->value(usernameKey).toString(),
                mSettings->value(passwordKey).toString(),

                mSettings->value(updateIntervalKey, 5).toInt(),
                mSettings->value(timeoutKey, 30).toInt(),

                mSettings->value(autoReconnectEnabledKey, false).toBool(),
                mSettings->value(autoReconnectIntervalKey, 30).toInt()
            },
            MountedDirectory::fromVariant(mSettings->value(mountedDirectoriesKey)),
            LastTorrents::fromVariant(mSettings->value(lastTorrentsKey)),
            mSettings->value(lastDownloadDirectoriesKey).toStringList(),
            mSettings->value(lastDownloadDirectoryKey).toString()
        };
        mSettings->endGroup();
        return server;
    }

    void Servers::updateMountedDirectories() {
        mSettings->beginGroup(currentServerName());
        mCurrentServerMountedDirectories = MountedDirectory::fromVariant(mSettings->value(mountedDirectoriesKey));
        mSettings->endGroup();
    }

    void Servers::sync() { mSettings->sync(); }
}
