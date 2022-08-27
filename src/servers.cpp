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

#include "servers.h"

#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QStringBuilder>

#include "libtremotesf/target_os.h"
#include "libtremotesf/torrent.h"

namespace tremotesf
{
    namespace
    {
        constexpr QSettings::Format settingsFormat = [] {
            if constexpr (isTargetOsWindows) {
                return QSettings::IniFormat;
            } else {
                return QSettings::NativeFormat;
            }
        }();

        const QLatin1String fileName("servers");

        const QLatin1String currentServerKey("current");
        const QLatin1String addressKey("address");
        const QLatin1String portKey("port");
        const QLatin1String apiPathKey("apiPath");

        const QLatin1String proxyTypeKey("proxyType");
        const QLatin1String proxyHostnameKey("proxyHostname");
        const QLatin1String proxyPortKey("proxyPort");
        const QLatin1String proxyUserKey("proxyUser");
        const QLatin1String proxyPasswordKey("proxyPassword");

        const QLatin1String httpsKey("https");
        const QLatin1String selfSignedCertificateEnabledKey("selfSignedCertificateEnabled");
        const QLatin1String selfSignedCertificateKey("selfSignedCertificate");
        const QLatin1String clientCertificateEnabledKey("clientCertificateEnabled");
        const QLatin1String clientCertificateKey("clientCertificate");

        const QLatin1String authenticationKey("authentication");
        const QLatin1String usernameKey("username");
        const QLatin1String passwordKey("password");

        const QLatin1String updateIntervalKey("updateInterval");
        const QLatin1String timeoutKey("timeout");

        const QLatin1String autoReconnectEnabledKey("autoReconnectEnabled");
        const QLatin1String autoReconnectIntervalKey("autoReconnectInterval");

        const QLatin1String mountedDirectoriesKey("mountedDirectories");
        const QLatin1String addTorrentDialogDirectoriesKey("addTorrentDialogDirectories");
        const QLatin1String lastTorrentsKey("lastTorrents");

        const QLatin1String localCertificateKey("localCertificate");

        const QLatin1String proxyTypeDefault("Default");
        const QLatin1String proxyTypeHttp("HTTP");
        const QLatin1String proxyTypeSocks5("SOCKS5");

        Server::ProxyType proxyTypeFromSettings(const QString& value)
        {
            if (value.isEmpty() || value == proxyTypeDefault) {
                return Server::ProxyType::Default;
            }
            if (value == proxyTypeHttp) {
                return Server::ProxyType::Http;
            }
            if (value == proxyTypeSocks5) {
                return Server::ProxyType::Socks5;
            }
            return Server::ProxyType::Default;
        }

        QLatin1String proxyTypeToSettings(Server::ProxyType type)
        {
            switch (type) {
            case Server::ProxyType::Default:
                return proxyTypeDefault;
            case Server::ProxyType::Http:
                return proxyTypeHttp;
            case Server::ProxyType::Socks5:
                return proxyTypeSocks5;
            }
            return proxyTypeDefault;
        }

        void migrateFromAccounts()
        {
            QSettings accounts(settingsFormat,
                               QSettings::UserScope,
                               qApp->organizationName(),
                               QLatin1String("accounts"));
            if (!accounts.childGroups().isEmpty()) {
                if (QFile::copy(accounts.fileName(),
                                QSettings(settingsFormat,
                                          QSettings::UserScope,
                                          qApp->organizationName(),
                                          fileName)
                                    .fileName())) {
                    QFile::remove(accounts.fileName());
                }
            }
        }
    }

    Server::Server(const QString& name,
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
                   const QVariant& addTorrentDialogDirectories)
        : libtremotesf::Server{name,
                               address,
                               port,
                               apiPath,

                               proxyType,
                               proxyHostname,
                               proxyPort,
                               proxyUser,
                               proxyPassword,

                               https,
                               selfSignedCertificateEnabled,
                               selfSignedCertificate,
                               clientCertificateEnabled,
                               clientCertificate,

                               authentication,
                               username,
                               password,

                               updateInterval,
                               timeout,

                               autoReconnectEnabled,
                               autoReconnectInterval},
          mountedDirectories(mountedDirectories),
          lastTorrents(lastTorrents),
          addTorrentDialogDirectories(addTorrentDialogDirectories)
    {

    }

    Servers* Servers::instance()
    {
        static auto* const instance = new Servers(qApp);
        return instance;
    }

    void Servers::migrate()
    {
        migrateFromAccounts();
    }

    bool Servers::hasServers() const
    {
        return !mSettings->childGroups().isEmpty();
    }

    std::vector<Server> Servers::servers()
    {
        std::vector<Server> list;
        const QStringList groups(mSettings->childGroups());
        list.reserve(static_cast<size_t>(groups.size()));
        for (const QString& group : groups) {
            list.push_back(getServer(group));
        }
        return list;
    }

    Server Servers::currentServer() const
    {
        return getServer(currentServerName());
    }

    QString Servers::currentServerName() const
    {
        return mSettings->value(currentServerKey).toString();
    }

    QString Servers::currentServerAddress()
    {
        mSettings->beginGroup(currentServerName());
        QString address(mSettings->value(addressKey).toString());
        mSettings->endGroup();
        return address;
    }

    void Servers::setCurrentServer(const QString& name)
    {
        mSettings->setValue(currentServerKey, name);
        updateMountedDirectories();
        emit currentServerChanged();
    }

    bool Servers::currentServerHasMountedDirectories() const
    {
        return !mCurrentServerMountedDirectories.empty();
    }

    bool Servers::isUnderCurrentServerMountedDirectory(const QString& path) const
    {
        for (const std::pair<QString, QString>& directory : mCurrentServerMountedDirectories) {
            const int localSize = directory.first.size();
            if (path.startsWith(directory.first)) {
                if (path.size() == localSize || path[localSize] == '/') {
                    return true;
                }
            }
        }
        return false;
    }

    QString Servers::firstLocalDirectory() const
    {
        if (mCurrentServerMountedDirectories.empty()) {
            return QString();
        }
        return mCurrentServerMountedDirectories.front().first;
    }

    QString Servers::fromLocalToRemoteDirectory(const QString& path)
    {
        for (const std::pair<QString, QString>& directory : mCurrentServerMountedDirectories) {
            const int localSize = directory.first.size();
            if (path.startsWith(directory.first)) {
                if (path.size() == localSize || path[localSize] == '/') {
                    return directory.second % path.midRef(localSize);
                }
            }
        }
        return QString();
    }

    QString Servers::fromRemoteToLocalDirectory(const QString& path)
    {
        for (const std::pair<QString, QString>& directory : mCurrentServerMountedDirectories) {
            const int remoteSize = directory.second.size();
            if (path.startsWith(directory.second)) {
                if (path.size() == remoteSize || path[remoteSize] == '/') {
                    return directory.first % path.midRef(remoteSize);
                }
            }
        }
        return QString();
    }

    LastTorrents Servers::currentServerLastTorrents() const
    {
        mSettings->beginGroup(currentServerName());
        LastTorrents lastTorrents;
        const QVariant lastTorrentsVariant(mSettings->value(lastTorrentsKey));
        if (lastTorrentsVariant.isValid() && lastTorrentsVariant.type() == QVariant::List) {
            lastTorrents.saved = true;
            const QVariantList torrentVariants(lastTorrentsVariant.toList());
            lastTorrents.torrents.reserve(static_cast<size_t>(torrentVariants.size()));
            for (const QVariant& variant : torrentVariants) {
                const QVariantMap torrentMap(variant.toMap());
                lastTorrents.torrents.push_back({torrentMap[QLatin1String("hashString")].toString(),
                                                 torrentMap[QLatin1String("finished")].toBool()});
            }
        }
        mSettings->endGroup();
        return lastTorrents;
    }

    void Servers::saveCurrentServerLastTorrents(const libtremotesf::Rpc* rpc)
    {
        mSettings->beginGroup(currentServerName());
        QVariantList torrents;
        torrents.reserve(static_cast<QVariantList::size_type>(rpc->torrents().size()));
        for (const auto& torrent : rpc->torrents()) {
            torrents.push_back(QVariantMap{{QLatin1String("hashString"), torrent->hashString()},
                                           {QLatin1String("finished"), torrent->isFinished()}});
        }
        mSettings->setValue(lastTorrentsKey, torrents);
        mSettings->endGroup();
    }

    QStringList Servers::currentServerAddTorrentDialogDirectories() const
    {
        QStringList directories;
        mSettings->beginGroup(currentServerName());
        directories = mSettings->value(addTorrentDialogDirectoriesKey).toStringList();
        mSettings->endGroup();
        return directories;
    }

    void Servers::setCurrentServerAddTorrentDialogDirectories(const QStringList& directories)
    {
        mSettings->beginGroup(currentServerName());
        mSettings->setValue(addTorrentDialogDirectoriesKey, directories);
        mSettings->endGroup();
    }

    void Servers::setServer(const QString& oldName,
                            const QString& name,
                            const QString& address,
                            int port,
                            const QString& apiPath,

                            Server::ProxyType proxyType,
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

                            const QVariantMap& mountedDirectories)
    {
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

        QStringList addTorrentDialogDirectories;
        if (!oldName.isEmpty() && name != oldName) {
            addTorrentDialogDirectories = mSettings->value(oldName % QLatin1Char('/') % addTorrentDialogDirectoriesKey).toStringList();
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

        mSettings->setValue(mountedDirectoriesKey, mountedDirectories);
        mSettings->setValue(addTorrentDialogDirectoriesKey, addTorrentDialogDirectories);

        mSettings->endGroup();

        if (currentChanged) {
            updateMountedDirectories(mountedDirectories);
            emit currentServerChanged();
        }

        if (oldName.isEmpty() && mSettings->childGroups().size() == 1) {
            emit hasServersChanged();
        }
    }

    void Servers::removeServer(const QString& name)
    {
        mSettings->remove(name);
        const QStringList Servers(mSettings->childGroups());
        if (Servers.isEmpty()) {
            setCurrentServer(QString());
            emit hasServersChanged();
        } else if (name == currentServerName()) {
            setCurrentServer(Servers.first());
        }
    }

    void Servers::saveServers(const std::vector<Server>& servers, const QString& current)
    {
        const bool hadServers = hasServers();
        mSettings->clear();
        mSettings->setValue(currentServerKey, current);
        for (const Server& server : servers) {
            mSettings->beginGroup(server.name);

            mSettings->setValue(addressKey, server.address);
            mSettings->setValue(portKey, server.port);
            mSettings->setValue(apiPathKey, server.apiPath);

            mSettings->setValue(proxyTypeKey, proxyTypeToSettings(server.proxyType));
            mSettings->setValue(proxyHostnameKey, server.proxyHostname);
            mSettings->setValue(proxyPortKey, server.proxyPort);
            mSettings->setValue(proxyUserKey, server.proxyUser);
            mSettings->setValue(proxyPasswordKey, server.proxyPassword);

            mSettings->setValue(httpsKey, server.https);
            mSettings->setValue(selfSignedCertificateEnabledKey, server.selfSignedCertificateEnabled);
            mSettings->setValue(selfSignedCertificateKey, server.selfSignedCertificate);
            mSettings->setValue(clientCertificateEnabledKey, server.clientCertificateEnabled);
            mSettings->setValue(clientCertificateKey, server.clientCertificate);

            mSettings->setValue(authenticationKey, server.authentication);
            mSettings->setValue(usernameKey, server.username);
            mSettings->setValue(passwordKey, server.password);

            mSettings->setValue(updateIntervalKey, server.updateInterval);
            mSettings->setValue(timeoutKey, server.timeout);

            mSettings->setValue(autoReconnectEnabledKey, server.autoReconnectEnabled);
            mSettings->setValue(autoReconnectIntervalKey, server.autoReconnectInterval);

            mSettings->setValue(mountedDirectoriesKey, server.mountedDirectories);
            mSettings->setValue(lastTorrentsKey, server.lastTorrents);
            mSettings->setValue(addTorrentDialogDirectoriesKey, server.addTorrentDialogDirectories);

            mSettings->endGroup();
        }
        updateMountedDirectories();
        emit currentServerChanged();
        if (hasServers() != hadServers) {
            emit hasServersChanged();
        }
    }

    Servers::Servers(QObject* parent)
        : QObject(parent),
          mSettings(new QSettings(settingsFormat,
                                  QSettings::UserScope,
                                  qApp->organizationName(),
                                  QLatin1String("servers"),
                                  this))
    {
        if (hasServers()) {
            bool setFirst = true;
            const QString current(currentServerName());
            if (!current.isEmpty()) {
                const QStringList groups(mSettings->childGroups());
                for (const QString& group : groups) {
                    if (group == current) {
                        setFirst = false;
                        break;
                    }
                }
            }
            if (setFirst) {
                setCurrentServer(mSettings->childGroups().constFirst());
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

    Server Servers::getServer(const QString& name) const
    {
        mSettings->beginGroup(name);
        Server server(mSettings->group(),
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
                      mSettings->value(autoReconnectIntervalKey, 30).toInt(),

                      mSettings->value(mountedDirectoriesKey).toMap(),
                      mSettings->value(lastTorrentsKey),
                      mSettings->value(addTorrentDialogDirectoriesKey));
        mSettings->endGroup();
        return server;
    }

    void Servers::updateMountedDirectories(const QVariantMap& directories)
    {
        mCurrentServerMountedDirectories.clear();
        mCurrentServerMountedDirectories.reserve(static_cast<size_t>(directories.size()));
        for (auto i = directories.cbegin(), end = directories.cend(); i != end; ++i) {
            mCurrentServerMountedDirectories.emplace_back(QDir(i.key()).absolutePath(), QDir(i.value().toString()).absolutePath());
        }
    }

    void Servers::updateMountedDirectories()
    {
        mSettings->beginGroup(currentServerName());
        updateMountedDirectories(mSettings->value(mountedDirectoriesKey).toMap());
        mSettings->endGroup();
    }
}
