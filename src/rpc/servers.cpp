// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servers.h"

#include <ranges>

#include <QCoreApplication>
#include <QSettings>
#include <QSslCertificate>
#include <QStringBuilder>

#include "log/log.h"
#include "rpc/pathutils.h"
#include "rpc/qsslcertificateformatter.h"
#include "rpc/serversettings.h"
#include "target_os.h"

using namespace Qt::StringLiterals;

SPECIALIZE_FORMATTER_FOR_QDEBUG(QVariant)

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

        constexpr auto serverCertificateModeKey = "serverCertificateMode"_L1;
        constexpr auto serverCertificateModeNone = "none"_L1;
        constexpr auto serverCertificateModeSelfSigned = "selfSigned"_L1;
        constexpr auto serverCertificateModeCustomRoot = "customRoot"_L1;

        constexpr auto serverRootCertificateKey = "serverRootCertificate"_L1;
        constexpr auto serverLeafCertificateKey = "serverLeafCertificate"_L1;

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

        constexpr QLatin1String proxyTypeToSettings(ConnectionConfiguration::ProxyType type) {
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

        ConnectionConfiguration::ServerCertificateMode serverCertificateModeFromSettings(const QString& value) {
            if (value.isEmpty() || value == serverCertificateModeNone) {
                return ConnectionConfiguration::ServerCertificateMode::None;
            }
            if (value == serverCertificateModeSelfSigned) {
                return ConnectionConfiguration::ServerCertificateMode::SelfSigned;
            }
            if (value == serverCertificateModeCustomRoot) {
                return ConnectionConfiguration::ServerCertificateMode::CustomRoot;
            }
            return ConnectionConfiguration::ServerCertificateMode::None;
        }

        constexpr QLatin1String serverCertificateModeToSettings(ConnectionConfiguration::ServerCertificateMode mode) {
            switch (mode) {
            case ConnectionConfiguration::ServerCertificateMode::None:
                return serverCertificateModeNone;
            case ConnectionConfiguration::ServerCertificateMode::SelfSigned:
                return serverCertificateModeSelfSigned;
            case ConnectionConfiguration::ServerCertificateMode::CustomRoot:
                return serverCertificateModeCustomRoot;
            }
            return serverCertificateModeNone;
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
        const ConnectionConfiguration& connectionConfiguration,
        std::vector<MountedDirectory> mountedDirectories
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

        mSettings->setValue(addressKey, connectionConfiguration.address);
        mSettings->setValue(portKey, connectionConfiguration.port);
        mSettings->setValue(apiPathKey, connectionConfiguration.apiPath);

        mSettings->setValue(proxyTypeKey, proxyTypeToSettings(connectionConfiguration.proxyType));
        mSettings->setValue(proxyHostnameKey, connectionConfiguration.proxyHostname);
        mSettings->setValue(proxyPortKey, connectionConfiguration.proxyPort);
        mSettings->setValue(proxyUserKey, connectionConfiguration.proxyUser);
        mSettings->setValue(proxyPasswordKey, connectionConfiguration.proxyPassword);

        mSettings->setValue(httpsKey, connectionConfiguration.https);

        mSettings->setValue(
            serverCertificateModeKey,
            serverCertificateModeToSettings(connectionConfiguration.serverCertificateMode)
        );
        mSettings->setValue(serverRootCertificateKey, connectionConfiguration.serverRootCertificate);
        mSettings->setValue(serverLeafCertificateKey, connectionConfiguration.serverLeafCertificate);

        mSettings->setValue(clientCertificateEnabledKey, connectionConfiguration.clientCertificateEnabled);
        mSettings->setValue(clientCertificateKey, connectionConfiguration.clientCertificate);

        mSettings->setValue(authenticationKey, connectionConfiguration.authentication);
        mSettings->setValue(usernameKey, connectionConfiguration.username);
        mSettings->setValue(passwordKey, connectionConfiguration.password);

        mSettings->setValue(updateIntervalKey, connectionConfiguration.updateInterval);
        mSettings->setValue(timeoutKey, connectionConfiguration.timeout);

        mSettings->setValue(autoReconnectEnabledKey, connectionConfiguration.autoReconnectEnabled);
        mSettings->setValue(autoReconnectEnabledKey, connectionConfiguration.autoReconnectInterval);

        mSettings->setValue(mountedDirectoriesKey, MountedDirectory::toVariant(mountedDirectories));
        mSettings->setValue(lastDownloadDirectoriesKey, lastDownloadDirectories);
        mSettings->setValue(lastDownloadDirectoryKey, lastDownloadDirectory);

        mSettings->endGroup();

        if (currentChanged) {
            mCurrentServerMountedDirectories = std::move(mountedDirectories);
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
                serverCertificateModeKey,
                serverCertificateModeToSettings(server.connectionConfiguration.serverCertificateMode)
            );
            mSettings->setValue(serverRootCertificateKey, server.connectionConfiguration.serverRootCertificate);
            mSettings->setValue(serverLeafCertificateKey, server.connectionConfiguration.serverLeafCertificate);

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
            migrateClientCertificateSettings();
            migrateServerCertificateSettings();
            mSettings->endGroup();
        }

        updateMountedDirectories();
    }

    Server Servers::getServer(const QString& name) const {
        mSettings->beginGroup(name);
        Server server{
            .name = mSettings->group(),
            .connectionConfiguration =
                ConnectionConfiguration{
                    .address = mSettings->value(addressKey).toString(),
                    .port = mSettings->value(portKey).toInt(),
                    .apiPath = mSettings->value(apiPathKey).toString(),

                    .proxyType = proxyTypeFromSettings(mSettings->value(proxyTypeKey).toString()),
                    .proxyHostname = mSettings->value(proxyHostnameKey).toString(),
                    .proxyPort = mSettings->value(proxyPortKey).toInt(),
                    .proxyUser = mSettings->value(proxyUserKey).toString(),
                    .proxyPassword = mSettings->value(proxyPasswordKey).toString(),

                    .https = mSettings->value(httpsKey, false).toBool(),

                    .serverCertificateMode =
                        serverCertificateModeFromSettings(mSettings->value(serverCertificateModeKey).toString()),
                    .serverRootCertificate = mSettings->value(serverRootCertificateKey).toByteArray(),
                    .serverLeafCertificate = mSettings->value(serverLeafCertificateKey).toByteArray(),

                    .clientCertificateEnabled = mSettings->value(clientCertificateEnabledKey, false).toBool(),
                    .clientCertificate = mSettings->value(clientCertificateKey).toByteArray(),

                    .authentication = mSettings->value(authenticationKey, false).toBool(),
                    .username = mSettings->value(usernameKey).toString(),
                    .password = mSettings->value(passwordKey).toString(),

                    .updateInterval = mSettings->value(updateIntervalKey, 5).toInt(),
                    .timeout = mSettings->value(timeoutKey, 30).toInt(),

                    .autoReconnectEnabled = mSettings->value(autoReconnectEnabledKey, false).toBool(),
                    .autoReconnectInterval = mSettings->value(autoReconnectIntervalKey, 30).toInt()
                },
            .mountedDirectories = MountedDirectory::fromVariant(mSettings->value(mountedDirectoriesKey)),
            .lastTorrents = LastTorrents::fromVariant(mSettings->value(lastTorrentsKey)),
            .lastDownloadDirectories = mSettings->value(lastDownloadDirectoriesKey).toStringList(),
            .lastDownloadDirectory = mSettings->value(lastDownloadDirectoryKey).toString()
        };
        mSettings->endGroup();
        return server;
    }

    void Servers::updateMountedDirectories() {
        mSettings->beginGroup(currentServerName());
        mCurrentServerMountedDirectories = MountedDirectory::fromVariant(mSettings->value(mountedDirectoriesKey));
        mSettings->endGroup();
    }

    void Servers::migrateClientCertificateSettings() {
        if (!mSettings->contains(localCertificateKey)) {
            return;
        }
        info().log("Migrating legacy client certificate settings for server {}", mSettings->group());
        const auto localCertificateValue = mSettings->value(localCertificateKey);
        const auto localCertificate = localCertificateValue.toByteArray();
        info().log("{} = {}", localCertificateKey, localCertificateValue);
        info().log("Parsed certificate:\n{}", QSslCertificate(localCertificate, QSsl::Pem));
        if (!localCertificate.isEmpty()) {
            setValueVerbose(clientCertificateEnabledKey, true);
            setValueVerbose(clientCertificateKey, localCertificate);
        } else {
            warning().log("{} is empty", localCertificateKey);
        }
        mSettings->remove(localCertificateKey);
    }

    namespace {
        bool isCertificateIssuedBy(const QSslCertificate& cert, const QSslCertificate& potentialCa) {
            if (cert == potentialCa) return false;
            const auto issuerAttrs = cert.issuerInfoAttributes();
            if (issuerAttrs.isEmpty()) return false;
            for (const auto& attr : issuerAttrs) {
                if (cert.issuerInfo(attr) != potentialCa.subjectInfo(attr)) {
                    return false;
                }
            }
            return true;
        }

        std::optional<QSslCertificate> findLeafCertificate(std::span<const QSslCertificate> chain) {
            // First, find first non self-signed certificate
            auto maybeLeaf =
                std::ranges::find_if(chain, [](const QSslCertificate& cert) { return !cert.isSelfSigned(); });
            if (maybeLeaf == chain.end()) {
                return std::nullopt;
            }
            // Go up the chain to skip any intermediary certificates if they are present (we don't need them here but user can add them too)
            while (true) {
                auto issued = std::ranges::find_if(chain, [&](const QSslCertificate& cert) {
                    return isCertificateIssuedBy(cert, *maybeLeaf);
                });
                if (issued != chain.end()) {
                    maybeLeaf = issued;
                } else {
                    break;
                }
            }
            if (maybeLeaf == chain.end()) {
                return std::nullopt;
            }
            return *maybeLeaf;
        }
    }

    void Servers::migrateServerCertificateSettings() {
        if (!mSettings->contains(selfSignedCertificateEnabledKey)) {
            return;
        }
        info().log("Migrating legacy server certificate settings for server {}", mSettings->group());
        const auto selfSignedCertificateValue = mSettings->value(selfSignedCertificateKey);
        info().log("{} = {}", selfSignedCertificateKey, selfSignedCertificateValue);
        const auto certs = QSslCertificate::fromData(selfSignedCertificateValue.toByteArray());
        info().log("Parsed {} certificates from {}", certs.size(), selfSignedCertificateKey);
        for (const auto& cert : certs) {
            info().log(cert);
        }
        if (!certs.isEmpty()) {
            const bool enabled = mSettings->value(selfSignedCertificateEnabledKey, false).toBool();
            if (!enabled) {
                setValueVerbose(
                    serverCertificateModeKey,
                    serverCertificateModeToSettings(ConnectionConfiguration::ServerCertificateMode::None)
                );
            }
            if (certs.size() == 1) {
                if (enabled) {
                    setValueVerbose(
                        serverCertificateModeKey,
                        serverCertificateModeToSettings(ConnectionConfiguration::ServerCertificateMode::SelfSigned)
                    );
                }
                setValueVerbose(serverRootCertificateKey, selfSignedCertificateValue);
            } else {
                if (enabled) {
                    setValueVerbose(
                        serverCertificateModeKey,
                        serverCertificateModeToSettings(ConnectionConfiguration::ServerCertificateMode::CustomRoot)
                    );
                }
                // Find the root
                if (const auto root = std::ranges::find_if(certs, &QSslCertificate::isSelfSigned);
                    root != certs.end()) {
                    debug().log("Root certificate:\n{}", *root);
                    setValueVerbose(serverRootCertificateKey, root->toPem());
                } else {
                    warning().log("Did not find a root CA certificate in connection configuration");
                }
                if (const auto leaf = findLeafCertificate(certs); leaf.has_value()) {
                    debug().log("Leaf certificate:\n{}", *leaf);
                    setValueVerbose(serverLeafCertificateKey, leaf->toPem());
                }
            }
        }
        mSettings->remove(selfSignedCertificateEnabledKey);
        mSettings->remove(selfSignedCertificateKey);
    }

    void Servers::setValueVerbose(QLatin1String key, const QVariant& value) {
        info().log("Setting {} to {}", key, value);
        mSettings->setValue(key, value);
    }

    void Servers::sync() { mSettings->sync(); }
}
