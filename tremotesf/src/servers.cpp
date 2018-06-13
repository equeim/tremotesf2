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
#include <QSettings>
#include <QStandardPaths>

namespace tremotesf
{
    namespace
    {
#ifdef Q_OS_WIN
        const QSettings::Format settingsFormat = QSettings::IniFormat;
#else
        const QSettings::Format settingsFormat = QSettings::NativeFormat;
#endif

        const QString fileName(QLatin1String("servers"));

        const QString versionKey(QLatin1String("version"));
        const QString currentServerKey(QLatin1String("current"));
        const QString addressKey(QLatin1String("address"));
        const QString portKey(QLatin1String("port"));
        const QString apiPathKey(QLatin1String("apiPath"));
        const QString httpsKey(QLatin1String("https"));
        const QString selfSignedCertificateEnabledKey(QLatin1String("selfSignedCertificateEnabled"));
        const QString selfSignedCertificateKey(QLatin1String("selfSignedCertificate"));
        const QString clientCertificateEnabledKey(QLatin1String("clientCertificateEnabled"));
        const QString clientCertificateKey(QLatin1String("clientCertificate"));
        const QString authenticationKey(QLatin1String("authentication"));
        const QString usernameKey(QLatin1String("username"));
        const QString passwordKey(QLatin1String("password"));
        const QString updateIntervalKey(QLatin1String("updateInterval"));
        const QString backgroundUpdateIntervalKey(QLatin1String("backgroundUpdateInterval"));
        const QString timeoutKey(QLatin1String("timeout"));

        const QString localCertificateKey(QLatin1String("localCertificate"));

        Servers* instancePointer = nullptr;

#ifdef TREMOTESF_SAILFISHOS
        void migrateFrom0()
        {
            QSettings settings;
            if (settings.value(versionKey).toInt() != 1) {
                QSettings serversSettings(qApp->organizationName(), fileName);
                if (serversSettings.childGroups().isEmpty()) {
                    for (const QString& group : settings.childGroups()) {
                        settings.beginGroup(group);
                        serversSettings.beginGroup(group);

                        serversSettings.setValue(addressKey, settings.value(addressKey));
                        serversSettings.setValue(portKey, settings.value(portKey));
                        serversSettings.setValue(apiPathKey, settings.value(apiPathKey));
                        serversSettings.setValue(httpsKey, settings.value(httpsKey));
                        if (settings.value(localCertificateKey).toBool()) {
                            const QString localCertificatePath(QStandardPaths::locate(QStandardPaths::DataLocation,
                                                                                      QString::fromLatin1("%1.pem").arg(group)));
                            if (!localCertificatePath.isEmpty()) {
                                QFile file(localCertificatePath);
                                if (file.open(QFile::ReadOnly)) {
                                    serversSettings.setValue(clientCertificateEnabledKey, true);
                                    serversSettings.setValue(clientCertificateKey, file.readAll());
                                }
                            }
                        }
                        serversSettings.setValue(authenticationKey, settings.value(authenticationKey));
                        serversSettings.setValue(usernameKey, settings.value(usernameKey));
                        serversSettings.setValue(passwordKey, settings.value(passwordKey));
                        serversSettings.setValue(updateIntervalKey, settings.value(updateIntervalKey));
                        serversSettings.setValue(timeoutKey, settings.value(timeoutKey));

                        serversSettings.endGroup();
                        settings.endGroup();
                    }
                    serversSettings.setValue(currentServerKey, settings.value(QLatin1String("currentAccount")));
                }
                settings.clear();
                settings.setValue(versionKey, 1);
            }
        }
#endif
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

    Servers* Servers::instance()
    {
        if (!instancePointer) {
            instancePointer = new Servers(qApp);
        }
        return instancePointer;
    }

    void Servers::migrate()
    {
#ifdef TREMOTESF_SAILFISHOS
        migrateFrom0();
#endif
        migrateFromAccounts();
    }

    bool Servers::hasServers() const
    {
        return !mSettings->childGroups().isEmpty();
    }

    std::vector<Server> Servers::servers()
    {
        std::vector<Server> list;
        for (const QString& group : mSettings->childGroups()) {
            list.push_back(getServer(group));
        }
        return list;
    }

    Server Servers::currentServer()
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
        const QString address(mSettings->value(addressKey).toString());
        mSettings->endGroup();
        return address;
    }

    void Servers::setCurrentServer(const QString& name)
    {
        mSettings->setValue(currentServerKey, name);
        emit currentServerChanged();
    }

    void Servers::setServer(const QString& oldName,
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
                            int timeout)
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

        if (!oldName.isEmpty() && name != oldName) {
            mSettings->remove(oldName);
        }

        mSettings->beginGroup(name);
        mSettings->setValue(addressKey, address);
        mSettings->setValue(portKey, port);
        mSettings->setValue(apiPathKey, apiPath);
        mSettings->setValue(httpsKey, https);
        mSettings->setValue(selfSignedCertificateEnabledKey, selfSignedCertificateEnabled);
        mSettings->setValue(selfSignedCertificateKey, selfSignedCertificate);
        mSettings->setValue(clientCertificateEnabledKey, clientCertificateEnabled);
        mSettings->setValue(clientCertificateKey, clientCertificate);
        mSettings->setValue(authenticationKey, authentication);
        mSettings->setValue(usernameKey, username);
        mSettings->setValue(passwordKey, password);
        mSettings->setValue(updateIntervalKey, updateInterval);
        mSettings->setValue(backgroundUpdateIntervalKey, backgroundUpdateInterval);
        mSettings->setValue(timeoutKey, timeout);
        mSettings->endGroup();

        if (currentChanged) {
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
            mSettings->setValue(httpsKey, server.https);
            mSettings->setValue(selfSignedCertificateEnabledKey, server.selfSignedCertificateEnabled);
            mSettings->setValue(selfSignedCertificateKey, server.selfSignedCertificate);
            mSettings->setValue(clientCertificateEnabledKey, server.clientCertificateEnabled);
            mSettings->setValue(clientCertificateKey, server.clientCertificate);
            mSettings->setValue(authenticationKey, server.authentication);
            mSettings->setValue(usernameKey, server.username);
            mSettings->setValue(passwordKey, server.password);
            mSettings->setValue(updateIntervalKey, server.updateInterval);
            mSettings->setValue(backgroundUpdateIntervalKey, server.backgroundUpdateInterval);
            mSettings->setValue(timeoutKey, server.timeout);
            mSettings->endGroup();
        }
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
                for (const QString& group : mSettings->childGroups()) {
                    if (group == current) {
                        setFirst = false;
                        break;
                    }
                }
            }
            if (setFirst) {
                setCurrentServer(mSettings->childGroups().first());
            }
        } else {
            mSettings->remove(currentServerKey);
        }

        for (const QString& group : mSettings->childGroups()) {
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
    }

    Server Servers::getServer(const QString& name)
    {
        mSettings->beginGroup(name);
        const Server server{
            mSettings->group(),
            mSettings->value(addressKey).toString(),
            mSettings->value(portKey).toInt(),
            mSettings->value(apiPathKey).toString(),
            mSettings->value(httpsKey, false).toBool(),
            mSettings->value(selfSignedCertificateEnabledKey, false).toBool(),
            mSettings->value(selfSignedCertificateKey).toByteArray(),
            mSettings->value(clientCertificateEnabledKey, false).toBool(),
            mSettings->value(clientCertificateKey).toByteArray(),
            mSettings->value(authenticationKey, false).toBool(),
            mSettings->value(usernameKey).toString(),
            mSettings->value(passwordKey).toString(),
            mSettings->value(updateIntervalKey, 5).toInt(),
            mSettings->value(backgroundUpdateIntervalKey, 30).toInt(),
            mSettings->value(timeoutKey, 30).toInt()};
        mSettings->endGroup();
        return server;
    }
}
